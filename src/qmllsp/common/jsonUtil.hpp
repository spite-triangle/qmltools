#ifndef JSONUTIL_HPP
#define JSONUTIL_HPP

#include "common/lspDefine.h"

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
};

#endif // JSONUTIL_HPP
