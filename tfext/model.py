import os
from absl import logging, app

import tensorflow as tf
from tensorflow.core.protobuf.config_pb2 import ConfigProto

from tensorflow.keras.initializers import GlorotNormal
from tensorflow.keras.layers import Dense

from tfext.local_cluster import LocalCluster
from tfext.pass_utils import add_custom_graph_optimizer
from tfext.utils import Graph, ReplicaSubGraph

user_num, item_num = 1024, 256


def input_fn(batch_size: int = 128):
  dataset = tf.data.TextLineDataset(filenames=['data/test'])
  dataset = dataset.map(lambda line: tf.strings.to_number(tf.strings.split(line, ','), out_type=tf.int64))
  return dataset.batch(batch_size=batch_size).prefetch(buffer_size=tf.data.AUTOTUNE)


def main(_):
  with tf.compat.v1.device("/job:ps/task:0"):
    partitioner=tf.compat.v1.fixed_size_partitioner(num_shards=4)
    user_emb = tf.compat.v1.get_variable(name='user_embeddings',
                                         shape=(user_num, 8),
                                         dtype=tf.float32,
                                         use_resource=True,
                                         initializer=GlorotNormal(),
                                         partitioner=partitioner)
  with tf.compat.v1.device("/job:ps/task:1"):
    item_emb = tf.compat.v1.get_variable(name='item_embeddings',
                                         shape=(item_num, 8),
                                         dtype=tf.float32,
                                         use_resource=True,
                                         initializer=GlorotNormal())
  
  with tf.compat.v1.device("/job:cpu_wk/task:0"):
    dataset0 = input_fn()
    iterator_0: tf.data.Iterator = tf.compat.v1.data.make_initializable_iterator(dataset0)
    data_0 = iterator_0.get_next()
    usr_feat_0, item_feat_0, label_0 = tf.split(data_0, num_or_size_splits=3, axis=1)
    
    usr_feat_0 = tf.nn.embedding_lookup(params=user_emb, ids=usr_feat_0, name='lu_usr_emb')
    item_feat_0 = tf.nn.embedding_lookup(params=item_emb, ids=item_feat_0, name='lu_item_emb')
    feats_0 = tf.concat([usr_feat_0, item_feat_0], axis=1, name='feats_0')
  
  # with tf.compat.v1.device("/job:cpu_wk/task:1"):
  #   dataset1 = input_fn()
  #   iterator_1: tf.data.Iterator = tf.compat.v1.data.make_initializable_iterator(dataset1)
  #   data_1 = iterator_1.get_next()
  #   usr_feat_1, item_feat_1, label_1 = tf.split(data_1, num_or_size_splits=3, axis=1)

  #   usr_feat_1 = tf.nn.embedding_lookup(params=user_emb, ids=usr_feat_1, name='lu_usr_emb')
  #   item_feat_1 = tf.nn.embedding_lookup(params=item_emb, ids=item_feat_1, name='lu_item_emb')
  #   feats_1 = tf.concat([usr_feat_1, item_feat_1], axis=1, name='feats_1')
  
  with tf.compat.v1.device("/job:chief"):
    # feats = tf.concat([feats_0, feats_1], axis=0, name='feats')
    # labels = tf.concat([label_0, label_1], axis=0, name='labels')

    graph_view = Graph(feats_0.graph.as_graph_def())
    target = [feats_0, label_0]
    replica_sub_graph = graph_view.replica_device(starts=target, devices=["/job:cpu_wk/task:1"])
    print('ip_names', replica_sub_graph.ip_names)
    with open('/Users/fitz/code/notebook/tfext/device.pbtxt', 'w') as fid:
      fid.write(str(replica_sub_graph.graph_def))
    input_map = {name: feats_0.graph.get_tensor_by_name(name) for name in replica_sub_graph.ip_names}
    print('input_map', input_map)
    return_elements = tf.compat.v1.import_graph_def(
      graph_def=replica_sub_graph.graph_def, input_map=input_map, 
      return_elements=tf.compat.v1.nest.flatten(replica_sub_graph.op_names), name='')
    print(dir(feats_0.graph))
    replica_sub_graph_inits = [feats_0.graph.get_operation_by_name(name) for name in replica_sub_graph.inits]
    merged = []
    for t, x in zip(target, return_elements):
      merged.append(tf.concat([t, x], axis=0))
    feats, labels = merged
    print('iterator_0.initializer', iterator_0.initializer)
    with open('/Users/fitz/code/notebook/tfext/device2.pbtxt', 'w') as fid:
      fid.write(str(feats_0.graph.as_graph_def()))
    layer1 = Dense(units=32, activation='relu')(feats)
    logits = Dense(units=1, activation='relu')(layer1)
    predict = tf.reshape(tf.nn.sigmoid(logits), shape=(-1,))
    loss = tf.losses.binary_crossentropy(y_true=labels, y_pred=predict, from_logits=False)
    with open('/Users/fitz/code/notebook/tfext/forward.pbtxt', 'w') as fid:
      fid.write(str(loss.graph.as_graph_def()))
    opt = tf.compat.v1.train.AdagradOptimizer(learning_rate=0.001)
    global_step = tf.compat.v1.train.get_or_create_global_step()
    train_op = opt.minimize(loss, global_step=global_step)

  cluster_conf = {'ps': 2, 'cpu_wk': 2, 'chief': 1}
  with LocalCluster(cluster_conf=cluster_conf) as cluster:
    config = ConfigProto()
    add_custom_graph_optimizer(config, name='StaticRegister', 
      parameters={'has_jobs': ['chief'], 
                  'has_fetch_ops': ['global_step'],
                  'filter_fetch_ops': ['init', 'identity_RetVal']})
    # MonitoredSession(
    #   session_creator=ChiefSessionCreator(master=..., config=...))
    with tf.compat.v1.Session(target=cluster.master.target, config=config) as sess:
      saver = tf.compat.v1.train.Saver(var_list=tf.compat.v1.global_variables())
      sess.run([tf.compat.v1.global_variables_initializer(), 
                iterator_0.initializer] + replica_sub_graph_inits)  # iterator_1.initializer
      with open('/Users/fitz/code/notebook/tfext/graph_full.pbtxt', 'w') as fid:
        fid.write(str(sess.graph.as_graph_def()))

      while True:
        try:
          _, gs = sess.run([train_op, global_step])
          print(gs)
        except tf.errors.OutOfRangeError:
          saver.save(sess=sess, save_path='/Users/fitz/code/notebook/tfext/ckpt/model', global_step=global_step)
          break

  # ckpt = tf.train.Checkpoint(step=tf.Variable(1), optimizer=opt, net=net, iterator=iterator)
  # manager = tf.train.CheckpointManager(ckpt, './tf_ckpts', max_to_keep=3)

  


if __name__ == '__main__':
  tf.compat.v1.disable_eager_execution()
  app.run(main)
