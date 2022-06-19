#include <emscripten/bind.h>

#include <iostream>

#include "client.cc"
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
  Model(std::string buffer) : state(nullptr), buffer(buffer) {
    loadModelFromBuffer();
  }

  ~Model() { STT_FreeModel(state); }

  int getSampleRate() const { return STT_GetModelSampleRate(this->state); }

  int getModelBeamWidth() const { return STT_GetModelBeamWidth(this->state); }

  int setModelBeamWidth(unsigned int width) const {
    return STT_SetModelBeamWidth(this->state, width);
  }

  void freeModel() const { return STT_FreeModel(this->state); }

  int enableExternalScorer(std::string scorerBuffer) const {
    return STT_EnableExternalScorerFromBuffer(this->state, scorerBuffer.c_str(),
                                              scorerBuffer.size());
  }

  int disableExternalScorer() const {
    return STT_DisableExternalScorer(this->state);
  }

  int setScorerAlphaBeta(float alpha, float beta) const {
    return STT_SetScorerAlphaBeta(this->state, alpha, beta);
  }

  int addHotWord(const std::string& word, float boost) {
    return STT_AddHotWord(this->state, word.c_str(), boost);
  }

  int eraseHotWord(const std::string& word) {
    return STT_EraseHotWord(this->state, word.c_str());
  }

  int clearHotWords() { return STT_ClearHotWords(this->state); }

  std::string speechToText(std::vector<short> audioBuffer) const {
    char* tempResult =
        STT_SpeechToText(this->state, audioBuffer.data(), audioBuffer.size());
    if (!tempResult) {
      // There was some error, return an empty string.
      return std::string();
    }

    // We must manually free the string if something was returned to us.
    std::string result = tempResult;
    STT_FreeString(tempResult);
    return result;
  }

  std::string speechToTextWithMetadata(std::vector<short> audioBuffer,
                                       unsigned int aNumResults) const {
    Metadata* tempResult = STT_SpeechToTextWithMetadata(
        this->state, audioBuffer.data(), audioBuffer.size(), aNumResults);

    if (!tempResult) {
      // There was some error, return an empty string.
      return std::string();
    }

    std::string jsonResult = MetadataToJSON(tempResult);
    STT_FreeMetadata(tempResult);

    return jsonResult;
  }

 private:
  ModelState* state;
  std::string buffer;

  void loadModelFromBuffer() {
    std::cout << "Loading model from buffer" << std::endl;
    int ret = STT_CreateModelFromBuffer(this->buffer.c_str(),
                                        this->buffer.size(), &this->state);
    if (ret != STT_ERR_OK) {
      char* error = STT_ErrorCodeToErrorMessage(ret);
      std::cerr << "Could not create model: " << error << std::endl;
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
      .function("getModelBeamWidth", &Model::getModelBeamWidth)
      .function("setModelBeamWidth", &Model::setModelBeamWidth)
      .function("freeModel", &Model::freeModel)
      .function("addHotWord", &Model::addHotWord)
      .function("eraseHotWord", &Model::eraseHotWord)
      .function("clearHotWords", &Model::clearHotWords)
      .function("speechToText", &Model::speechToText)
      .function("speechToTextWithMetadata", &Model::speechToTextWithMetadata,
                allow_raw_pointers())
      .function("enableExternalScorer", &Model::enableExternalScorer)
      .function("disableExternalScorer", &Model::disableExternalScorer)
      .function("setScorerAlphaBeta", &Model::setScorerAlphaBeta);

  register_vector<short>("VectorShort");
}
