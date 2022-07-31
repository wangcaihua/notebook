import os
from typing import Dict, List, Optional, Any

from tensorflow.python.framework.load_library import load_library
from tensorflow.core.framework.attr_value_pb2 import AttrValue
from tensorflow.core.protobuf.config_pb2 import ConfigProto

lob_path = os.path.join(os.path.dirname(__file__), "graph_optimization_pass.so")
lib_handle = load_library(lob_path)


def to_attr_value(value: Any) -> AttrValue:
  result = AttrValue()
  if isinstance(value, (list, tuple)):
    for ele in value:
      if isinstance(ele, int):
        result.list.i.append(ele)
      elif isinstance(ele, str):
        result.list.s.append(ele)
      elif isinstance(ele, float):
        result.list.f.append(ele)
      elif isinstance(ele, bool):
        result.list.b.append(ele)
      else:
        raise Exception("data type in list not supported")
      result.list.s.append(ele)
  elif isinstance(value, str):
    result.s = value
  elif isinstance(value, int):
    result.i = value
  elif isinstance(value, float):
    result.f = value
  elif isinstance(value, bool):
    result.b = value
  else:
    raise Exception("data type not supported")
  return result


def add_custom_graph_optimizer(config: ConfigProto, name: str, parameters: Dict[str, Any]):
  rewriter_config = config.graph_options.rewrite_options
  # rewriter_config.disable_meta_optimizer = False
  custom_graph_optimizer = rewriter_config.custom_optimizers.add()

  custom_graph_optimizer.name = name
  parameter_map = custom_graph_optimizer.parameter_map
  for key, value in parameters.items():
    parameter_map[key].CopyFrom(to_attr_value(value))
