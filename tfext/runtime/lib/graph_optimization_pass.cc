#include <unordered_set>
#include <memory>
#include <algorithm>

#include "absl/strings/str_split.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/graph/graph.h"
#include "tensorflow/core/graph/node_builder.h"


#include "tensorflow/core/common_runtime/optimization_registry.h"
#include "tensorflow/core/platform/stacktrace.h"
#include "tensorflow/core/util/dump_graph.h"
#include "tensorflow/core/public/session_options.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/grappler/grappler_item.h"
#include "tensorflow/core/grappler/optimizers/custom_graph_optimizer.h"
#include "tensorflow/core/grappler/optimizers/custom_graph_optimizer_registry.h"
#include "third_party/nlohmann/json.hpp"

namespace tensorflow {
using json = nlohmann::json;

void GetCurrentTask(std::string *type, int *index) {
  const char *env_p = std::getenv("TF_CONFIG");
  if (env_p != nullptr) {
    auto tf_config = json::parse(env_p);
    if (tf_config.find("task") != tf_config.end()) {
      *type = tf_config["task"]["type"];
      *index = tf_config["task"]["index"];
    }
  }
}


class RepeatSubGraphPass : public GraphOptimizationPass {
 public:
  Status Run(const GraphOptimizationPassOptions& options) override {
    if (options.graph == nullptr && options.partition_graphs == nullptr) {
      return Status::OK();
    } else if (options.is_function_graph) {
      return Status::OK();
    } else {
      std::unique_ptr<tensorflow::Graph> *graph = options.graph;
      // std::string file_name = "/Users/fitz/code/notebook/tfext/graph_full.pbtxt";
      // TF_RETURN_IF_ERROR(WriteGraphDef(file_name, graph));
      const ConfigProto &config = options.session_options->config;
      LOG(INFO) << config.DebugString();

      std::string type = "";
      int index = -1;
      GetCurrentTask(&type, &index);
      LOG(INFO) << "type is " << type << ", index is " << index;
      Env *env = Env::Default();

      return Status::OK();
    }
    // std::string stackTrace = CurrentStackTrace();
    // LOG(INFO) << stackTrace;
  }

 private:
  Env *env_ = Env::Default();

  Status WriteGraphDef(std::string file_name, std::unique_ptr<tensorflow::Graph> *graph) {
    std::unique_ptr<WritableFile> ostream;
    GraphDef graph_def;
    (*graph)->ToGraphDef(&graph_def);
    TF_RETURN_IF_ERROR(env_->NewWritableFile(file_name, &ostream));
    TF_RETURN_IF_ERROR(ostream->Append(graph_def.DebugString()));
    TF_RETURN_IF_ERROR(ostream->Close());

    return Status::OK();
  }

};

REGISTER_OPTIMIZATION(OptimizationPassRegistry::POST_PLACEMENT, 0,
                      RepeatSubGraphPass);


namespace grappler {

static const char* kTestOptimizerName = "Test";
static const char* kTestPluginOptimizerName = "TestPlugin";

class TestGraphOptimizer : public CustomGraphOptimizer {
 public:
  Status Init(const tensorflow::RewriterConfig_CustomGraphOptimizer* config) override {
    if (config != nullptr) {
      name_ = config->name();
      const auto &parameter_map = config->parameter_map();
      if (parameter_map.count("has_jobs") != 0) {
        const auto &list = parameter_map.at("has_jobs").list().s();
        has_jobs_.insert(list.begin(), list.end());
      }
      LOG_FIRST_N(INFO, 1) << "has_jobs: [" << absl::StrJoin(has_jobs_, ",") << "]";
      
      if (parameter_map.count("has_fetch_ops") != 0) {
        const auto &list = parameter_map.at("has_fetch_ops").list().s();
        has_fetch_ops_.insert(list.begin(), list.end());
      }
      LOG_FIRST_N(INFO, 1) << "has_fetch_ops: [" << absl::StrJoin(has_fetch_ops_, ",") << "]";

      if (parameter_map.count("filter_fetch_ops") != 0) {
        const auto &list = parameter_map.at("filter_fetch_ops").list().s();
        filter_fetch_ops_.insert(list.begin(), list.end());
      }
      LOG_FIRST_N(INFO, 1) << "filter_fetch_ops: [" << absl::StrJoin(filter_fetch_ops_, ",") << "]";

      if (parameter_map.count("select_fetch_ops") != 0) {
        const auto &list = parameter_map.at("select_fetch_ops").list().s();
        select_fetch_ops_.insert(list.begin(), list.end());
      }
      LOG_FIRST_N(INFO, 1) << "select_fetch_ops: [" << absl::StrJoin(select_fetch_ops_, ",") << "]";

      int index = -1;
      GetCurrentTask(&job_type_, &index);
      LOG_FIRST_N(INFO, 1) << "job_type: " << job_type_ << ", index: " << index;
    } else {
      name_ = "TestGraphOptimizer";
    }

    return Status::OK();
  }

  string name() const override { return name_; }

  bool UsesFunctionLibrary() const override { return false; }

  Status Optimize(Cluster* cluster, const GrapplerItem& item,
                  GraphDef* optimized_graph) override {
    // LOG(INFO) << optimized_graph->DebugString();
    // std::string stackTrace = CurrentStackTrace();
    // LOG(INFO) << stackTrace;

    if (NeedRewrite(item)) {
      LOG(INFO) << "NeedRewrite ...";
      optimized_graph->CopyFrom(item.graph);
    } else {
      optimized_graph->CopyFrom(item.graph);
    }

    return Status::OK();
  }

  virtual void Feedback(Cluster* cluster, const GrapplerItem& item,
                const GraphDef& optimized_graph, double result) override {
  }

 private:
  std::string job_type_ = "";
  std::unordered_set<std::string> has_jobs_;
  std::unordered_set<std::string> has_fetch_ops_;
  std::unordered_set<std::string> filter_fetch_ops_;
  std::unordered_set<std::string> select_fetch_ops_;
  std::string name_;

  bool NeedRewrite(const GrapplerItem& item){
    if (!has_jobs_.empty() && has_jobs_.count(job_type_) == 0) {
      return false;
    }

    bool has_fetch_ops = select_fetch_ops_.empty();
    for (const std::string &fetch: item.fetch) {
      if (!filter_fetch_ops_.empty() && filter_fetch_ops_.count(fetch) != 0) {
        return false;
      } else if (!select_fetch_ops_.empty() && select_fetch_ops_.count(fetch) == 0) {
        return false;
      } else if (!has_fetch_ops_.empty() && has_fetch_ops_.count(fetch) != 0) {
        has_fetch_ops = true;
      }
    }

    return has_fetch_ops;
  }
};


class TestPluginGraphOptimizer : public CustomGraphOptimizer {
 public:
  Status Init(
      const tensorflow::RewriterConfig_CustomGraphOptimizer* config) override {
    return Status::OK();
  }
  string name() const override { return kTestPluginOptimizerName; }
  bool UsesFunctionLibrary() const override { return false; }
  Status Optimize(Cluster* cluster, const GrapplerItem& item,
                  GraphDef* optimized_graph) override {
    return Status::OK();
  }
};


REGISTER_GRAPH_OPTIMIZER_AS(TestGraphOptimizer, "StaticRegister");
}  // namespace grappler
}  // namespace tensorflow
