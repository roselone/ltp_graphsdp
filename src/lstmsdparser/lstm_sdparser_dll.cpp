#include "lstmsdparser/lstm_sdparser_dll.h"
#include "lstmsdparser/lstm_sdparser.h"
#include <iostream>

class __ltp_dll_lstmsdparser_wrapper : public ltp::lstmsdparser::LSTMParser {
public:
  __ltp_dll_lstmsdparser_wrapper() {}
  ~__ltp_dll_lstmsdparser_wrapper() {}

  bool load(const char * model_file) {
    if (!ltp::lstmsdparser::LSTMParser::load_model(std::string(model_file))) {
      return false;
    }
    return true;
  }

  int parse(const std::vector<std::string> & words,
            const std::vector<std::string> & postags,
            std::vector<std::vector<std::string>> & hyp) {
    // hyp[i][j] means the head of word i is word j
    // hyp0[i][j] means the arc from word i to word j
    std::vector<std::vector<std::string>> hyp0;

    // add ROOT to the end of the sentence
    std::vector<std::string> _words = words;
    std::vector<std::string> _postags = postags;
    _words.push_back("ROOT");
    _postags.push_back("ROOT");

    ltp::lstmsdparser::LSTMParser::predict(hyp0, _words, _postags);
    for (unsigned i = 0; i < hyp0.size() - 1; i++) {
        std::vector<std::string> r;
        for (unsigned j = 0; j < hyp0.size(); j++) r.push_back(ltp::lstmsdparser::REL_NULL);
        hyp.push_back(r);
    }

    // transform from hyp0 to hyp
    for (int i = 0; i < hyp0.size(); i++){
      for (int j = 0; j < hyp0.size(); j++){
        if (hyp0[i][j] != ltp::lstmsdparser::REL_NULL)
          hyp[j][i] = hyp0[i][j];
      }
    }
    return hyp.size();
  }
};

void * lstmsdparser_create_parser(const char * model_dir) {
  __ltp_dll_lstmsdparser_wrapper* wrapper = new __ltp_dll_lstmsdparser_wrapper();
  std::string model_file = model_dir;
  wrapper -> Opt.USE_BILSTM = true;
  wrapper -> Opt.USE_TREELSTM = true;
  wrapper -> Opt.PRETRAINED_DIM = 50;
  if (!wrapper->load(model_file.c_str())) {
    delete wrapper;
    return 0;
  }
  return reinterpret_cast<void *>(wrapper);
}

int lstmsdparser_release_parser(void * parser) {
  if (!parser) {
    return -1;
  }
  delete reinterpret_cast<__ltp_dll_lstmsdparser_wrapper*>(parser);
  return 0;
}

int lstmsdparser_parse(void * parser,
                 const std::vector<std::string> & words,
                 const std::vector<std::string> & postags,
                 std::vector<std::vector<std::string>> & hyp) {
  if (words.size() != postags.size()) {
    return 0;
  }
  for (int i = 0; i < words.size(); ++ i) {
    if (words[i].empty() || postags[i].empty()) {
      return 0;
    }
  }

  __ltp_dll_lstmsdparser_wrapper* wrapper = 0;
  wrapper = reinterpret_cast<__ltp_dll_lstmsdparser_wrapper*>(parser);
  return wrapper->parse(words, postags, hyp);
}

int lstmsdparser_parse(void * parser,
                       const std::vector<std::string> & words,
                       const std::vector<std::string> & postags,
                       std::vector<std::vector<std::pair<int, std::string>>> & hyp) {
  std::vector<std::vector<std::string>> vecSemResult;
  int ret = lstmsdparser_parse(parser, words, postags, vecSemResult);
  if (ret <= 0) {
    return -1;
  }

  for (int i = 0; i < words.size(); ++ i) {
    std::vector<std::pair<int, std::string>> wordSemResult;
    for (int idx = 0; idx < vecSemResult.size() + 1; ++ idx) {
      if (vecSemResult[i][idx] != "-NULL-"){
        if (vecSemResult[i][idx] == "Root")
          wordSemResult.push_back(std::make_pair(-1, std::string("Root")));
        else
          wordSemResult.push_back(std::make_pair(idx, vecSemResult[i][idx]));
      }
    }
    hyp.push_back(std::move(wordSemResult));
  }

  return ret;
}
