#pragma once

#include "system_control.h"

#include "hippomocks.h"

#include <boost/optional.hpp>

#include <string>
#include <memory>

namespace test {

class SystemControl;

class ControlRequest: public ctrl_request {
public:
    void freeRequestData();
    std::string requestData() const;
    bool hasRequestData() const;

    int allocReplyData(size_t size);
    void freeReplyData();
    std::string replyData() const;
    bool hasReplyData() const;

    void setResult(int result);
    int result() const;
    bool hasResult() const;

    // This class is non-copyable
    ControlRequest(const ControlRequest&) = delete;
    ControlRequest& operator=(const ControlRequest&) = delete;

private:
    std::string reqData_, repData_;
    boost::optional<int> result_;

    ControlRequest(uint16_t type, std::string data, SystemControl* channel);

    friend class SystemControl;
};

class SystemControl {
public:
    explicit SystemControl(MockRepository* mocks);

    std::shared_ptr<ControlRequest> makeRequest(uint16_t type, std::string data = std::string());
};

} // namespace test

inline test::ControlRequest::ControlRequest(uint16_t type, std::string data, SystemControl* channel) :
        ctrl_request{ sizeof(ctrl_request), type, nullptr, 0, nullptr, 0, channel },
        reqData_(std::move(data)) {
    if (!reqData_.empty()) {
        request_data = &reqData_.front();
        request_size = reqData_.size();
    }
}

inline void test::ControlRequest::freeRequestData() {
    reqData_ = std::string();
    request_data = nullptr;
    request_size = 0;
}

inline std::string test::ControlRequest::requestData() const {
    return reqData_.substr(0, request_size);
}

inline bool test::ControlRequest::hasRequestData() const {
    return (request_size > 0);
}

inline int test::ControlRequest::allocReplyData(size_t size) {
    repData_.resize(size);
    reply_data = &repData_.front();
    reply_size = size;
    return SYSTEM_ERROR_NONE;
}

inline void test::ControlRequest::freeReplyData() {
    repData_ = std::string();
    reply_data = nullptr;
    reply_size = 0;
}

inline std::string test::ControlRequest::replyData() const {
    return repData_.substr(0, reply_size);
}

inline bool test::ControlRequest::hasReplyData() const {
    return (reply_size > 0);
}

inline void test::ControlRequest::setResult(int result) {
    result_ = result;
}

inline int test::ControlRequest::result() const {
    return (result_ ? *result_ : SYSTEM_ERROR_UNKNOWN);
}

inline bool test::ControlRequest::hasResult() const {
    return (bool)result_;
}

inline test::SystemControl::SystemControl(MockRepository* mocks) {
    // system_ctrl_alloc_reply_data()
    mocks->OnCallFunc(system_ctrl_alloc_reply_data).Do([](ctrl_request* req, size_t size, void* reserved) {
        const auto r = static_cast<ControlRequest*>(req);
        return r->allocReplyData(size);
    });
    // system_ctrl_free_reply_data()
    mocks->OnCallFunc(system_ctrl_free_reply_data).Do([](ctrl_request* req, void* reserved) {
        const auto r = static_cast<ControlRequest*>(req);
        r->freeReplyData();
    });
    // system_ctrl_free_request_data()
    mocks->OnCallFunc(system_ctrl_free_request_data).Do([](ctrl_request* req, void* reserved) {
        const auto r = static_cast<ControlRequest*>(req);
        r->freeRequestData();
    });
    // system_ctrl_set_result()
    mocks->OnCallFunc(system_ctrl_set_result).Do([](ctrl_request* req, int result, void* reserved) {
        const auto r = static_cast<ControlRequest*>(req);
        r->setResult(result);
    });
}

inline std::shared_ptr<test::ControlRequest> test::SystemControl::makeRequest(uint16_t type, std::string data) {
    return std::shared_ptr<ControlRequest>(new ControlRequest(type, std::move(data), this));
}