#include <unordered_map>
#include <memory>

#include "absl/strings/str_split.h"
#include "absl/strings/str_cat.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/graph/graph.h"
#include "tensorflow/core/graph/node_builder.h"


#include "tensorflow/core/common_runtime/optimization_registry.h"
#include "tensorflow/core/platform/stacktrace.h"
#include "tensorflow/core/util/dump_graph.h"
#include "tensorflow/core/public/session_options.h"
#include "tensorflow/core/platform/env.h"

#include "tensorflow/core/grappler/optimizers/custom_graph_optimizer.h"
#include "tensorflow/core/grappler/optimizers/custom_graph_optimizer_registry.h"


namespace tensorflow {

class RepeatSubGraphPass : public GraphOptimizationPass {
 public:
  Status Run(const GraphOptimizationPassOptions& options) override {
    if (options.graph == nullptr && options.partition_graphs == nullptr) {
      return Status::OK();
    } else if (options.is_function_graph) {
      return Status::OK();
    } else {
      std::unique_ptr<tensorflow::Graph> *graph = options.graph;
      std::string file_name = "/Users/fitz/code/notebook/tfext/graph_full.pbtxt";
      TF_RETURN_IF_ERROR(WriteGraphDef(file_name, graph));
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
    LOG(INFO) << "CustomGraphOptimizer: " << config->DebugString();
    return Status::OK();
  }

  string name() const override { return kTestOptimizerName; }

  bool UsesFunctionLibrary() const override { return false; }

  Status Optimize(Cluster* cluster, const GrapplerItem& item,
                  GraphDef* optimized_graph) override {
    LOG(INFO) << "TestGraphOptimizer called!...\n";
    LOG(INFO) << optimized_graph->DebugString();
    return Status::OK();
  }

  virtual void Feedback(Cluster* cluster, const GrapplerItem& item,
                const GraphDef& optimized_graph, double result) override {
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
