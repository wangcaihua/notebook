import os
from absl import logging, app

import tensorflow as tf
from tensorflow.core.protobuf.config_pb2 import ConfigProto

from tensorflow.keras.initializers import GlorotNormal
from tensorflow.keras.layers import Dense

from tfext.local_cluster import LocalCluster
from tfext.pass_utils import add_custom_graph_optimizer

user_num, item_num = 1024, 256


def input_fn(batch_size: int = 128):
  dataset = tf.data.TextLineDataset(filenames=['data/test'])
  dataset = dataset.map(lambda line: tf.strings.to_number(tf.strings.split(line, ','), out_type=tf.int64))
  return dataset.batch(batch_size=batch_size).prefetch(buffer_size=tf.data.AUTOTUNE)


def main(_):
  with tf.compat.v1.device("/job:ps/task:0"):
    user_emb = tf.compat.v1.get_variable(name='user_embeddings',
                                         shape=(user_num, 8),
                                         dtype=tf.float32,
                                         use_resource=True,
                                         initializer=GlorotNormal())
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
  
  with tf.compat.v1.device("/job:cpu_wk/task:1"):
    dataset1 = input_fn()
    iterator_1: tf.data.Iterator = tf.compat.v1.data.make_initializable_iterator(dataset1)
    data_1 = iterator_1.get_next()
    usr_feat_1, item_feat_1, label_1 = tf.split(data_1, num_or_size_splits=3, axis=1)

    usr_feat_1 = tf.nn.embedding_lookup(params=user_emb, ids=usr_feat_1, name='lu_usr_emb')
    item_feat_1 = tf.nn.embedding_lookup(params=item_emb, ids=item_feat_1, name='lu_item_emb')
    feats_1 = tf.concat([usr_feat_1, item_feat_1], axis=1, name='feats_1')
  
  with tf.compat.v1.device("/job:chief"):
    feats = tf.concat([feats_0, feats_1], axis=0, name='feats')
    labels = tf.concat([label_0, label_1], axis=0, name='labels')
    
    layer1 = Dense(units=32, activation='relu')(feats)
    logits = Dense(units=1, activation='relu')(layer1)
    predict = tf.reshape(tf.nn.sigmoid(logits), shape=(-1,))
    loss = tf.losses.binary_crossentropy(y_true=labels, y_pred=predict, from_logits=False)
    
    opt = tf.compat.v1.train.GradientDescentOptimizer(learning_rate=0.001)
    global_step = tf.compat.v1.train.get_or_create_global_step()
    train_op = opt.minimize(loss, global_step=global_step)

  cluster_conf = {'ps': 2, 'cpu_wk': 2, 'chief': 1}
  with LocalCluster(cluster_conf=cluster_conf) as cluster:
    config = ConfigProto()
    add_custom_graph_optimizer(config, name='StaticRegister', parameters={'hello': 1, 'word': 2})
    # MonitoredSession(
    #   session_creator=ChiefSessionCreator(master=..., config=...))
    with tf.compat.v1.Session(target=cluster.master.target, config=config) as sess:
      sess.run([tf.compat.v1.global_variables_initializer(), 
                iterator_0.initializer, iterator_1.initializer])

      while True:
        try:
          _, gs = sess.run([train_op, global_step])
          print(gs)
        except tf.errors.OutOfRangeError:
          break


if __name__ == '__main__':
  tf.compat.v1.disable_eager_execution()
  app.run(main)
