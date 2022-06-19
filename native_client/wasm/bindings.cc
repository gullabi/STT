#include <emscripten/bind.h>

#include <iostream>

#include "client.cc"
#include "coqui-stt.h"

using namespace emscripten;

class Stream {
 public:
  Stream(StreamingState* streamingState)
    : streamingState(streamingState) {}

  void feedAudioContent(std::vector<short> audioBuffer) {
    STT_FeedAudioContent(this->streamingState, audioBuffer.data(), audioBuffer.size());
  }

  std::string intermediateDecode() {
    char* tempResult = STT_IntermediateDecode(this->streamingState);
    if (!tempResult) {
      // There was some error, return an empty string.
      return std::string();
    }

    // We must manually free the string if something was returned to us.
    std::string result = tempResult;
    STT_FreeString(tempResult);
    return result;
  }

  // TODO: Actually return a wrapper to Metadata instead of a
  // stringified JSON.
  std::string intermediateDecodeWithMetadata(unsigned int numResults = 1) {
    Metadata* tempResult =
      STT_IntermediateDecodeWithMetadata(this->streamingState, numResults);
    if (!tempResult) {
      // There was some error, return an empty string.
      return std::string();
    }

    std::string jsonResult = MetadataToJSON(tempResult);
    STT_FreeMetadata(tempResult);

    return jsonResult;
  }

  std::string intermediateDecodeFlushBuffers() {
    char* tempResult =
      STT_IntermediateDecodeFlushBuffers(this->streamingState);
    if (!tempResult) {
      // There was some error, return an empty string.
      return std::string();
    }

    // We must manually free the string if something was returned to us.
    std::string result = tempResult;
    STT_FreeString(tempResult);
    return result;
  }

  std::string intermediateDecodeWithMetadataFlushBuffers(unsigned int numResults = 1) {
    Metadata* tempResult =
      STT_IntermediateDecodeWithMetadataFlushBuffers(this->streamingState, numResults);
    if (!tempResult) {
      // There was some error, return an empty string.
      return std::string();
    }

    std::string jsonResult = MetadataToJSON(tempResult);
    STT_FreeMetadata(tempResult);

    return jsonResult;
  }

  std::string finishStream() {
    char* tempResult = STT_FinishStream(this->streamingState);
    // Regardless of the result, the stream will be deleted.
    this->streamingState = nullptr;

    if (!tempResult) {
      // There was some error, return an empty string.
      return std::string();
    }

    // We must manually free the string if something was returned to us.
    std::string result = tempResult;
    STT_FreeString(tempResult);
    return result;
  }

  std::string finishStreamWithMetadata(unsigned int numResults = 1) {
    Metadata* tempResult =
      STT_FinishStreamWithMetadata(this->streamingState, numResults);
    // Regardless of the result, the stream will be deleted.
    this->streamingState = nullptr;

    if (!tempResult) {
      // There was some error, return an empty string.
      return std::string();
    }

    std::string jsonResult = MetadataToJSON(tempResult);
    STT_FreeMetadata(tempResult);

    return jsonResult;
  }

 private:
  StreamingState* streamingState;
};

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

  // TODO: Actually return a wrapper to Metadata instead of a
  // stringified JSON.
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

  Stream* createStream() {
    StreamingState* streamingState;
    int status = STT_CreateStream(this->state, &streamingState);
    if (status != STT_ERR_OK) {
      char* error = STT_ErrorCodeToErrorMessage(status);
      std::cerr << "createStream failed: " << error << std::endl;
      STT_FreeString(error);
      return nullptr;
    }

    return new Stream(streamingState);
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
EMSCRIPTEN_BINDINGS(coqui_ai_apis) {
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
      .function("createStream", &Model::createStream, allow_raw_pointers())
      .function("enableExternalScorer", &Model::enableExternalScorer)
      .function("disableExternalScorer", &Model::disableExternalScorer)
      .function("setScorerAlphaBeta", &Model::setScorerAlphaBeta);

  class_<Stream>("Stream")
      .constructor<StreamingState*>()
      .function("feedAudioContent", &Stream::feedAudioContent)
      .function("intermediateDecode", &Stream::intermediateDecode)
      .function("intermediateDecodeWithMetadata", &Stream::intermediateDecodeWithMetadata)
      .function("intermediateDecodeFlushBuffers", &Stream::intermediateDecodeFlushBuffers)
      .function("intermediateDecodeWithMetadataFlushBuffers",
                &Stream::intermediateDecodeWithMetadataFlushBuffers)
      .function("finishStream", &Stream::finishStream)
      .function("finishStreamWithMetadata", &Stream::finishStreamWithMetadata);

  register_vector<short>("VectorShort");
}
