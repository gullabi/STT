
#include <iostream>
#include <time.h>

#include <emscripten/bind.h>
#include "coqui-stt.h"

using namespace emscripten;

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
    : state(nullptr)
    , buffer(buffer)
  {
    loadModelFromBuffer();
  }

  ~Model() {
    STT_FreeModel(state);
  }

  int getSampleRate() const {
    return STT_GetModelSampleRate(this->state);
  }

  int enableExternalScorer(std::string scorerBuffer) const {
    return STT_EnableExternalScorerFromBuffer(this->state, scorerBuffer.c_str(), scorerBuffer.size());
  }

  std::string speechToText(std::vector<short> audioBuffer) const {
    clock_t start = clock();
    clock_t finish;
    std::cout << "Start STT_SpeechToText" << std::endl;
    char* tempResult = STT_SpeechToText(this->state, audioBuffer.data(), audioBuffer.size());
    if (!tempResult) {
      // There was some error, return an empty string.
      return std::string();
    }
    finish = clock();
    std::cout << "Finished STT_SpeechToText " << (finish - start) / CLOCKS_PER_SEC << std::endl;

    // We must manually free the string if something was returned to us.
    std::string result = tempResult;
    STT_FreeString(tempResult);
    return result;
  }

  //void setX(int x_) { x = x_; }

  /*static std::string getStringFromInstance(const Model& instance) {
    return instance.y;
  }*/

private:
  int x;
  ModelState* state;
  std::string buffer;

  void loadModelFromBuffer() {
    std::cout << "Loading model" << std::endl;
    std::cout << "clock per sec "<< CLOCKS_PER_SEC << std::endl;
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
    .function("getSampleRate", &Model::getSampleRate)
    .function("speechToText", &Model::speechToText)
    .function("enableExternalScorer", &Model::enableExternalScorer)
    //.property("x", &Model::getX, &Model::setX)
    //.class_function("getStringFromInstance", &Model::getStringFromInstance)
    ;

  register_vector<short>("VectorShort");
}
