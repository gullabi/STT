
#include <stdio.h>

#include <emscripten/bind.h>
#include "coqui-stt.h"

using namespace emscripten;

float lerp(float a, float b, float t) {
    return (1 - t) * a + t * b;
}
/*
bool CreateStream() {
    StreamingState* ctx;
    int status = STT_CreateStream(aCtx, &ctx);
    if (status != STT_ERR_OK) {
      return false;
    }
    return true;
}*/

class Model {
public:
  Model(std::string buffer)
    : buffer(buffer)
  {
    loadModelFromBuffer();
  }

  void incrementX() {
    ++x;
  }

  int getX() const { return x; }
  void setX(int x_) { x = x_; }

  /*static std::string getStringFromInstance(const Model& instance) {
    return instance.y;
  }*/

private:
  int x;
  ModelState* state;
  std::string buffer;

  void loadModelFromBuffer() {
    int ret = STT_CreateModelFromBuffer(this->buffer.c_str(), this->buffer.size(), &this->state);
    if (ret != STT_ERR_OK) {
      char* error = STT_ErrorCodeToErrorMessage(ret);
      fprintf(stderr, "Could not create model: %s\n", error);
      STT_FreeString(error);
      return;
    }
  }
};

// Binding code
EMSCRIPTEN_BINDINGS(my_class_example) {
  class_<Model>("Model")
    .constructor<std::string>()
    .function("incrementX", &Model::incrementX)
    .property("x", &Model::getX, &Model::setX)
    //.class_function("getStringFromInstance", &Model::getStringFromInstance)
    ;
}

EMSCRIPTEN_BINDINGS(my_module) {
    function("lerp", &lerp);
}

// TODO: STT_CreateStream

