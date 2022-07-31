import os
import json
from absl import logging
from copy import deepcopy
from multiprocessing import Process
from typing import Dict, List, Optional

import tensorflow as tf
from tfext.utils import get_random_ports


class Job(object):
  def __init__(self, cluster_dict, session_config, joinable: bool = True):
    self.cluster_dict = deepcopy(cluster_dict)
    self.session_config = deepcopy(session_config)
    self.server: Optional[tf.distribute.Server] = None
    self._joinable = joinable
    
    os.environ.pop('http_proxy', None)
    os.environ.pop('https_proxy', None)
  
  def __call__(self, job_name, task_index):
    os.environ['TF_CONFIG'] = json.dumps({
      'cluster': self.cluster_dict,
      'task': {
        'type': job_name,
        'index': task_index
      }
    })
    
    logging.info(f'start {job_name}, {task_index} at {self.cluster_dict[job_name][task_index]} ...')
    self.server = tf.distribute.Server(self.cluster_dict,
                                      job_name=job_name,
                                      task_index=task_index,
                                      protocol='grpc',
                                      config=self.session_config)
    if self._joinable:
      self.server.join()
    return self

  def __del__(self):
    self.cluster_dict = None
    self.session_config = None
    if self.server:
      del self.server

  @property
  def target(self):
    if self.server:
      return self.server.target
    else:
      return None


class LocalCluster(object):
  
  def __init__(self, cluster_conf: Dict[str, int], disable_meta_optimizer: bool = False):
    # fix CreateSession still waiting for response from worker
    assert 'chief' in cluster_conf
    self.cluster_dict: Dict[str, List[str]] = {}
    free_ports = get_random_ports(sum(cluster_conf.values()))
    for job, cnt in cluster_conf.items():
      self.cluster_dict[job] = [f'localhost:{free_ports.pop()}' for _ in range(cnt)]

    logging.info(f"cluster_dict: {self.cluster_dict}")
    self.cluster_spec: tf.train.ClusterSpec = tf.train.ClusterSpec(cluster=self.cluster_dict)
    self.session_config: tf.compat.v1.ConfigProto = tf.compat.v1.ConfigProto(
      cluster_def=self.cluster_spec.as_cluster_def(),
      allow_soft_placement=True,
      share_cluster_devices_in_session=True
    )
    self.session_config.experimental.share_session_state_in_clusterspec_propagation = True
    self.session_config.graph_options.rewrite_options.disable_meta_optimizer = disable_meta_optimizer
    
    self.servers: Dict[str, List[tf.distribute.Server]] = {}
    self.master: Optional[tf.distribute.Server] = None
    
  def __enter__(self):
    job, svc_addrs = 'ps', self.cluster_dict['ps']
    svc_list = [Process(target=Job(self.cluster_dict, self.session_config), kwargs={'job_name': job, 'task_index':index})
                for index, addr in enumerate(svc_addrs)]
    self.servers[job] = svc_list
    for job in svc_list:
      job.start()

    job, svc_addrs = 'cpu_wk', self.cluster_dict['cpu_wk']
    svc_list = [Process(target=Job(self.cluster_dict, self.session_config), kwargs={'job_name': job, 'task_index':index})
                for index, addr in enumerate(svc_addrs)]
    self.servers[job] = svc_list
    for job in svc_list:
      job.start()

    self.master = Job(self.cluster_dict, self.session_config, False)('chief', 0)
    return self
  
  def __exit__(self, exc_type, exc_val, exc_tb):
    del self.master
    for svc_list in self.servers.values():
      for svc in svc_list:
        svc.terminate()
        svc.join()

    self.servers: Dict[str, List[tf.distribute.Server]] = {}
    self.master: Optional[tf.distribute.Server] = None
