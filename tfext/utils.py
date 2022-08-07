import socket
from absl import logging
from copy import deepcopy
import tensorflow as tf
from typing import Dict, List, Set, Optional, Tuple
from collections import defaultdict, namedtuple
from dataclasses import dataclass, field
from google.protobuf import text_format


save_op_name = 'save/SaveV2'
init_op_name = 'init'
restore_op_name = 'save/restore_all'
VAR_OPS = {'VarHandleOp', 'VarIsInitializedOp', 
           'ReadVariableOp', 'AssignVariableOp'}


ReplicaSubGraph = namedtuple('ReplicaSubGraph', 'graph_def ip_names op_names inits')


def get_random_ports(num: int):
  socks, ports = [], []
  for _ in range(num):
    sock = socket.socket()
    sock.bind(('', 0))
    port = sock.getsockname()[1]
    ports.append(port)
    socks.append(sock)
  for sock in socks:
    sock.close()

  return ports


@dataclass
class Variable(object):
  handle: Optional[tf.compat.v1.NodeDef] = None
  is_initialized: Optional[tf.compat.v1.NodeDef] = None
  read: List[tf.compat.v1.NodeDef] = field(default_factory=list)
  update: List[tf.compat.v1.NodeDef] = field(default_factory=list)
  init: Optional[tf.compat.v1.NodeDef] = None
  restore: Optional[tf.compat.v1.NodeDef] = None


class Graph(object):
  def __init__(self, graph_def: tf.compat.v1.GraphDef):
    self._graph_def = deepcopy(graph_def)
    self._versions = self._graph_def.versions
    self._library = self._graph_def.library

    self._node_def: Dict[str, tf.compat.v1.NodeDef] = {
      self.node_name(node_def.name):node_def for node_def in self._graph_def.node}
    
    self._consumers: Dict[str, Set[str]] = defaultdict(set)
    
    self._ops: Dict[str, Set[str]] = defaultdict(set)
    for name, node_def in self._node_def.items():
      self._ops[node_def.op].add(name)
      for ip_name in node_def.input:
        self._consumers[self.node_name(ip_name)].add(name)

    self._variables: Dict[str, Variable] = None
    self._vars_related: Dict[str, tf.compat.v1.NodeDef] = None
    self._data_pipeline: Dict[str, List[tf.compat.v1.NodeDef]] = None

  @property
  def variables(self) -> Dict[str, List[tf.compat.v1.NodeDef]]:
    if not self._variables:
      self._variables = defaultdict(Variable)

      for op_name in VAR_OPS:
        for name in self._ops[op_name]:
          node_def = self._node_def[name]
          var_name = self.node_name(node_def.input[0]) if node_def.input else name

          if node_def.op == 'VarHandleOp':
            self._variables[var_name].handle = node_def
          elif node_def.op == 'VarIsInitializedOp':
            self._variables[var_name].is_initialized = node_def
          elif node_def.op == 'ReadVariableOp':
            self._variables[var_name].read.append(node_def)
          else:
            assert node_def.op == 'AssignVariableOp'
            assert len(node_def.input) == 2

            source = self.node_name(node_def.input[1])
            if source.startswith('save/'):
              assert name.startswith('save/')
              # for restore
              self._variables[var_name].restore = node_def
            elif '/Initializer/' in source:
              assert 'save/' not in name 
              # for initialize
              self._variables[var_name].init = node_def 
            else:
              # for update
              self._variables[var_name].update.append(node_def)

    return self._variables

  @property
  def vars_related(self) -> Dict[str, tf.compat.v1.NodeDef]:
    if self._vars_related is None:
      starts = [save_op_name, init_op_name, restore_op_name] + list(self.variables.keys())
      self._vars_related = self.sub_range(starts=starts)
    
    return self._vars_related

  @property
  def data_pipeline(self) -> Dict[str, List[tf.compat.v1.NodeDef]]:
    if self._data_pipeline is None:
      self._data_pipeline = {}
      for name in self._ops['IteratorV2']:
        node = self._node_def[name]
        if name not in self._data_pipeline:
          starts = list(set(node.input) | self._consumers[name])
          self._data_pipeline[name] = self.sub_range(starts=starts)
    
    return self._data_pipeline

  def sub_range(self, starts: List[str], skips: List[str] = None,
                extend_iter: bool = True, extend_var: bool = True) -> Dict[str, tf.compat.v1.NodeDef]:
    result = {}
    skips = [self.node_name(n) for n in skips] if skips else []
    queue: List[tf.compat.v1.NodeDef] = [self._node_def.get(name, None) for name in starts]
    while queue:
      node: tf.compat.v1.NodeDef = queue.pop()
      if node is None:
        continue
      name = self.node_name(node.name)
      if name in skips:
        continue

      if name in result:
        continue
      else:
        result[name] = node

      if node.input:
        for ip_name in node.input:
          ip_name = self.node_name(ip_name)
          if ip_name not in result:
            queue.append(self._node_def[ip_name])
      elif extend_iter and node.op == 'IteratorV2':
        for op_name in self._consumers[name]:
          if op_name not in result:
            node_def = self._node_def[op_name]
            if node_def.op != 'IteratorGetNext':
              queue.append(node_def)
      elif extend_var and node.op == 'VarHandleOp':
        for op_name in self._consumers[name]:
          if op_name not in result:
            node_def = self._node_def[op_name]
            if node_def.op in VAR_OPS:
              if node_def.op == 'ReadVariableOp':
                upnd = self.node_name(node_def.name)
                if upnd == f'{name}/Read/ReadVariableOp':
                  queue.append(node_def)
              else:
                queue.append(node_def)

    return result

  def replica_device(self, 
                     starts: List[str], 
                     devices: List[str],
                     skips: List[str] = None, 
                     extend_iter: bool = True, 
                     extend_var: bool = True) -> ReplicaSubGraph:
    if not devices or not starts:
      return (None, None)

    if skips is None:
      skips_ops = list(self.vars_related.keys()) 
    else:
      skips_ops = list(set(self.node_name(name) for name in skips) | set(self.vars_related.keys()))

    start_ops = {}
    for value in starts:
      if isinstance(value, (tf.Tensor, tf.Operation)):
        name = value.name
      else:
        assert isinstance(value, str)
        name = value

      idx = int(name[(name.rindex(':') + 1):]) if ':' in name else 0
      start_ops[self.node_name(name)] = idx

    origin = self.sub_range(list(start_ops), skips_ops, extend_iter, extend_var)
    
    nodes: List[tf.compat.v1.NodeDef] = []
    input_names = set()
    name_mapping: Dict[str, List[str]] = defaultdict(list)
    init_names = set()
    
    for i, device in enumerate(devices):
      for name, node_def in origin.items():
        node_def = deepcopy(node_def)
        node_def.name = self.new_name(node_def.name, i+1)

        node_def.device = device
        collocation = node_def.attr.get('_class')
        if collocation:
          loc_list = collocation.list.s
          assert len(loc_list) == 1
          raw_name = loc_list[0][5:].decode('utf-8')
          loc = self.node_name(raw_name)
          if loc in skips_ops:
            del node_def.attr['_class']
          elif loc in origin:
            new_loc = self.new_name(raw_name, i+1)
            del collocation.list.s[:]
            collocation.list.s.append(f'loc:@{new_loc}'.encode('utf-8'))
          else:
            raise Exception('location error')

        inputs = []
        for ip_name in node_def.input:
          if self.node_name(ip_name) in skips_ops:
            inputs.append(ip_name)
          else:
            inputs.append(self.new_name(ip_name, i+1))
        del node_def.input[:]
        node_def.input.extend(inputs)

        nodes.append(node_def)
        idx = start_ops.get(name, None)
        if idx is not None:
          name_mapping[name].append(f'{node_def.name}:{idx}')
        
        if node_def.op == 'MakeIterator':
          init_names.add(node_def.name)

    graph_def = tf.compat.v1.GraphDef()
    for node in nodes:
      node_def = graph_def.node.add()
      node_def.CopyFrom(node)
    
    
    known_names = {self.node_name(node.name) for node in nodes}
    for node in nodes:
      for ip_name in node.input:
        if self.node_name(ip_name) not in known_names:
          assert ip_name in self.vars_related
          if ':' in ip_name:
            input_names.add(ip_name)
          else:
            input_names.add(f'{ip_name}:0')
    
    return ReplicaSubGraph(graph_def, list(input_names), name_mapping, list(init_names))
    

  @classmethod
  def node_name(cls, name: str) -> str:
    if name.startswith('^'):
      name = name[1:]
    
    if ':' in name:
      idx = name.rindex(':')
      name = name[:idx]
    
    return name

  @classmethod
  def new_name(cls, name: str, idx: int) -> str:
    name_terms = name.split('/')
    if ':' in name_terms[0]:
      i = name_terms[0].rindex(':')
      name_terms[0] = f'{name_terms[0][:i]}_r_{idx}{name_terms[0][i:]}'
    else:
      name_terms[0] = f'{name_terms[0]}_r_{idx}'
    return '/'.join(name_terms)

  def to_graph_def(self, data) -> tf.compat.v1.GraphDef:
    graph_def = tf.compat.v1.GraphDef()

    nodes = set()
    if isinstance(data, (list, tuple, set)):
      for item in data:
        if isinstance(item, str):
          nodes.add(item)
        else:
          assert isinstance(item, tf.compat.v1.NodeDef)
          nodes.add(item.name)
    elif isinstance(data, dict):
      for key, value in data.items():
        nodes.add(key)
        if isinstance(value, str):
          nodes.add(value)
        elif isinstance(value, (list, tuple, set)):
          for item in value:
            if isinstance(item, str):
              nodes.add(item)
            else:
              assert isinstance(item, tf.compat.v1.NodeDef)
              nodes.add(item.name)
        elif isinstance(value, dict):
          for item in value.values():
            if isinstance(item, str):
              nodes.add(item)
            else:
              assert isinstance(item, tf.compat.v1.NodeDef)
              nodes.add(item.name)

    for name in nodes:
      nd = self._node_def.get(name, None)
      if nd is not None:
        node_def = graph_def.node.add()
        node_def.CopyFrom(nd)

    return graph_def


class ReplicaCtx(object):
  def __init__(self):
    self._before_ops: Set[str] = set()
    self._after_ops: Set[str] = set()
    self._target: Dict[str, List[int]] = defaultdict(list)

    self._has_collected = False
  
  def __enter__(self):
    graph = tf.compat.v1.get_default_graph()
    for op in graph.get_operations():
      self._before_ops.add(op.name)
    
    return self
  
  def __exit__(self, exc_type, exc_val, exc_tb):
    graph = tf.compat.v1.get_default_graph()
    for op in graph.get_operations():
      self._after_ops.add(op.name)
    self._has_collected = True

  @classmethod
  def _parse_tensor_name(cls, name: str) -> Tuple[str, int]:
    if name.startswith('^'):
      name = name[1:]

    if ':' in name:
      i = name.rindex(':')
      name = name[:i]
      idx = int(name[(i+1):])
    else:
      idx = 0
    
    return name, idx

  @classmethod
  def _node_name(cls, name: str) -> str:
    node_name, _ = cls._parse_tensor_name(name)
    return node_name

  @classmethod
  def _new_name(cls, name: str, idx: int) -> str:
    name_terms = name.split('/')
    if ':' in name_terms[0]:
      i = name_terms[0].rindex(':')
      name_terms[0] = f'{name_terms[0][:i]}_r_{idx}{name_terms[0][i:]}'
    else:
      name_terms[0] = f'{name_terms[0]}_r_{idx}'
    return '/'.join(name_terms)

  def add_target(self, value) -> 'ReplicaCtx':
    if value:
      if isinstance(value, (list, tuple, set)):
        for ele in value:
          if isinstance(ele, (tf.Tensor, tf.Operation)):
            assert not ele.name.startswith('^')
            node_name, idx = self._parse_tensor_name(ele.name)
            self._target[node_name].append(idx)
          else:
            assert isinstance(ele, str)
            assert not ele.startswith('^')
            node_name, idx = self._parse_tensor_name(ele)
            self._target[node_name].append(idx)
      else:
        if isinstance(value, (tf.Tensor, tf.Operation)):
          assert not value.name.startswith('^')
          node_name, idx = self._parse_tensor_name(value.name)
          self._target[node_name].append(idx)
        else:
          assert isinstance(value, str)
          assert not value.startswith('^')
          node_name, idx = self._parse_tensor_name(value)
          self._target[node_name].append(idx)

    return self

  def replica(self, devices: List[str]):
    if not self._has_collected or len(self._target) == 0:
      logging.warning('op not collected or no target ops!')
      return

    graph = tf.compat.v1.get_default_graph()
    diff_ops = self._after_ops - self._before_ops
    graph_def = tf.compat.v1.GraphDef()
    init_names = set()
    target_mapping: Dict[str, List[str]] = defaultdict(list)
    for i, device in enumerate(devices):
      for name in diff_ops:
        node = graph.get_operation_by_name(name)
        node_def = node.node_def

        # 1) change name
        node_def.name = self._new_name(node_def.name, i+1)

        # 2) change device
        node_def.device = device
        collocation = node_def.attr.get('_class')
        if collocation:
          loc_list = collocation.list.s
          assert len(loc_list) == 1
          raw_name = loc_list[0][5:].decode('utf-8')
          loc = self._node_name(raw_name)
          if loc in self._before_ops:
            del node_def.attr['_class']
          elif loc in diff_ops:
            new_loc = self.new_name(raw_name, i+1)
            del collocation.list.s[:]
            collocation.list.s.append(f'loc:@{new_loc}'.encode('utf-8'))
          else:
            raise Exception('location error')

        # 3) change input
        inputs = []
        for ip_name in node_def.input:
          if self._node_name(ip_name) in self._before_ops:
            inputs.append(ip_name)
          else:
            inputs.append(self._new_name(ip_name, i+1))
        del node_def.input[:]
        node_def.input.extend(inputs)

        # 4) copy to new graph_def
        new_node_def = graph_def.node.add()
        new_node_def.CopyFrom(node_def)

        idxs = self._target.get(name, [])
        for idx in idxs:
          target_mapping[name].append(f'{node_def.name}:{idx}')
        
        if node_def.op == 'MakeIterator':
          init_names.add(node_def.name)
    
    known_names = {self.node_name(node.name) for node in nodes}
    for node in nodes:
      for ip_name in node.input:
        if self.node_name(ip_name) not in known_names:
          assert ip_name in self.vars_related
          if ':' in ip_name:
            input_names.add(ip_name)
          else:
            input_names.add(f'{ip_name}:0')


if __name__ == '__main__':
  with open('/Users/fitz/code/notebook/tfext/forward.pbtxt', 'r') as fid:
    graph_def = tf.compat.v1.GraphDef()
    text_format.Parse(text=''.join(fid.readlines()), message=graph_def)
    gv = Graph(graph_def)
    for name, nd_list in gv.variables.items():
      print(nd_list)
      break
    with open('/Users/fitz/code/notebook/tfext/init.pbtxt', 'w') as ostream:
      ostream.write(str(gv.to_graph_def(gv.sub_range(starts=[init_op_name]))))
    with open('/Users/fitz/code/notebook/tfext/save.pbtxt', 'w') as ostream:
      ostream.write(str(gv.to_graph_def(gv.sub_range(starts=[save_op_name]))))
    with open('/Users/fitz/code/notebook/tfext/restore.pbtxt', 'w') as ostream:
      ostream.write(str(gv.to_graph_def(gv.sub_range(starts=[restore_op_name]))))
    with open('/Users/fitz/code/notebook/tfext/variables.pbtxt', 'w') as ostream:
      ostream.write(str(gv.to_graph_def(gv.sub_range(starts=[init_op_name, save_op_name, restore_op_name] + 
      list(gv.variables.keys())))))
    
    with open('/Users/fitz/code/notebook/tfext/feats.pbtxt', 'w') as ostream:
      vars_related = gv.sub_range(starts=[init_op_name, save_op_name, restore_op_name] + list(gv.variables.keys()))
      ostream.write(str(gv.to_graph_def(gv.sub_range(starts=['feats'], skips=list(vars_related.keys())))))
    # with open('/Users/fitz/code/notebook/tfext/data_pipeline.pbtxt', 'w') as ostream:
    #   ostream.write(str(gv.to_graph_def(gv.data_pipeline)))
