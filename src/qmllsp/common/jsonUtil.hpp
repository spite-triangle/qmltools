#ifndef JSONUTIL_HPP
#define JSONUTIL_HPP

#include <string>

#include "common/lspDefine.h"
#include "common/jsonSerializer.hpp"

class JsonUtil{

public:
    static Json ResponseMessge(const Json & req, const Json & result){
        return Json(
                {
                    {"jsonrpc", "2.0"},
                    { "id", req["id"]},
                    { "result", result }
                });
    }

    static Json ResponseError(const Json & req, const Json & error){
        return Json(
                {
                    {"jsonrpc", "2.0"},
                    { "id", req["id"]},
                    { "error", error}
                });
    }

    static Json NotificationMessage(const std::string & method, const Json & json){
        return Json(
                {
                    {"jsonrpc", "2.0"},
                    {"params", json},
                    {"method", method}
                });
    }

    static Json DiagnosticsMessage(const std::string & uri){
        return Json({
            {"uri", uri},
            {"diagnostics", Json::array()}
        });
    }

    static Json Diagnostics(const RANGE_S & range, DIAGNOSTIC_SEVERITY_E severity, const std::string & source, const std::string & msg){
        return Json({
            {"range", range},
            {"severity", severity},
            {"source", source},
            {"message", msg}
        });
    }
};

#endif // JSONUTIL_HPP
