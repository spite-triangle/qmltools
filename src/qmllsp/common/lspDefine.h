#ifndef LSPDEFINE_H
#define LSPDEFINE_H

#include <memory>

#include <QUrl>
#include <QString>

#include "common/json.hpp"

using Json = nlohmann::json;
using JsonPtr = std::shared_ptr<nlohmann::json>;

struct POSITION_S{
    uint64_t line = 0;
    uint64_t character = 0;
};

/* 区间取值范围为： [start, end) */
struct RANGE_S{
    POSITION_S start;
    POSITION_S end;  	
};

struct TEXT_DOCUMENT_ITEM_S{
    std::string uri;
    std::string languageId;
    int64_t version;
    std::string text;
};

enum DIAGNOSTIC_SEVERITY_E : int{
    DS_DEFAULT = 0,
    DS_ERROR = 1,
    DS_WARNING = 2,
    DS_INFORMATION = 3,
    DS_HINT = 4,
};

enum LSP_ERROR_E: int{
	// Defined by JSON-RPC
	 PARSE_ERROR = -32700,
	 INVALID_REQUEST = -32600,
	 METHOD_NOTFOUND = -32601,
	 INVALID_PARAMS = -32602,
	 INTERNAL_ERROR = -32603,

	/**
	 * This is the start range of JSON-RPC reserved error codes.
	 * It doesn't denote a real error code. No LSP error codes should
	 * be defined between the start and end range. For backwards
	 * compatibility the `ServerNotInitialized` and the `UnknownErrorCode`
	 * are left in the range.
	 *
	 * @since 3.16.0
	 */
	 JSONRPC_RESERVED_ERROR_RANGE_START = -32099,
	/** @deprecated use jsonrpcReservedErrorRangeStart */
	 SERVER_ERROR_START = JSONRPC_RESERVED_ERROR_RANGE_START,

	/**
	 * Error code indicating that a server received a notification or
	 * request before the server has received the `initialize` request.
	 */
	 SERVER_NOT_INITIALIZED = -32002,
	 UNKNOWN_ERROR_CODE = -32001,

	/**
	 * This is the end range of JSON-RPC reserved error codes.
	 * It doesn't denote a real error code.
	 *
	 * @since 3.16.0
	 */
	 JSONRPC_RESERVED_ERROR_RANGE_END = -32000,
	/** @deprecated use jsonrpcReservedErrorRangeEnd */
	 SERVER_ERROR_END = JSONRPC_RESERVED_ERROR_RANGE_END,

	/**
	 * This is the start range of LSP reserved error codes.
	 * It doesn't denote a real error code.
	 *
	 * @since 3.16.0
	 */
	 LSP_RESERVED_ERROR_RANGE_START = -32899,

	/**
	 * A request failed but it was syntactically correct, e.g the
	 * method name was known and the parameters were valid. The error
	 * message should contain human readable information about why
	 * the request failed.
	 *
	 * @since 3.17.0
	 */
	 REQUEST_FAILED = -32803,

	/**
	 * The server cancelled the request. This error code should
	 * only be used for requests that explicitly support being
	 * server cancellable.
	 *
	 * @since 3.17.0
	 */
	 SERVER_CANCELLED = -32802,

	/**
	 * The server detected that the content of a document got
	 * modified outside normal conditions. A server should
	 * NOT send this error code if it detects a content change
	 * in it unprocessed messages. The result even computed
	 * on an older state might still be useful for the client.
	 *
	 * If a client decides that a result is not of any use anymore
	 * the client should cancel the request.
	 */
	 CONTENT_MODIFIED = -32801,

	/**
	 * The client has canceled a request and a server has detected
	 * the cancel.
	 */
	 REQUEST_CANCELLED = -32800,

	/**
	 * This is the end range of LSP reserved error codes.
	 * It doesn't denote a real error code.
	 *
	 * @since 3.16.0
	 */
	 LSP_RESERVED_ERROR_RANGE_END = -32800,

};


#endif // LSPDEFINE_H
