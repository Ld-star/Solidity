/**
 * THIS FILE IS GENERATED BY jsonrpcstub, DO NOT CHANGE IT!!!!!
 */

#ifndef _WEBTHREESTUBCLIENT_H_
#define _WEBTHREESTUBCLIENT_H_

#include <jsonrpc/rpc.h>

class WebThreeStubClient
{
    public:
        WebThreeStubClient(jsonrpc::AbstractClientConnector* conn)
        {
            this->client = new jsonrpc::Client(conn);
        }
        ~WebThreeStubClient()
        {
            delete this->client;
        }

        std::string db_get(const std::string& param1, const std::string& param2) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);
p.append(param2);

            Json::Value result = this->client->CallMethod("db_get",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        std::string db_getString(const std::string& param1, const std::string& param2) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);
p.append(param2);

            Json::Value result = this->client->CallMethod("db_getString",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        bool db_put(const std::string& param1, const std::string& param2, const std::string& param3) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);
p.append(param2);
p.append(param3);

            Json::Value result = this->client->CallMethod("db_put",p);
    if (result.isBool())
        return result.asBool();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        bool db_putString(const std::string& param1, const std::string& param2, const std::string& param3) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);
p.append(param2);
p.append(param3);

            Json::Value result = this->client->CallMethod("db_putString",p);
    if (result.isBool())
        return result.asBool();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        Json::Value eth_accounts() throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p = Json::nullValue;
            Json::Value result = this->client->CallMethod("eth_accounts",p);
    if (result.isArray())
        return result;
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        std::string eth_balanceAt(const std::string& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_balanceAt",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        Json::Value eth_blockByHash(const std::string& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_blockByHash",p);
    if (result.isObject())
        return result;
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        Json::Value eth_blockByNumber(const int& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_blockByNumber",p);
    if (result.isObject())
        return result;
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        std::string eth_call(const Json::Value& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_call",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        bool eth_changed(const int& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_changed",p);
    if (result.isBool())
        return result.asBool();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        std::string eth_codeAt(const std::string& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_codeAt",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        std::string eth_coinbase() throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p = Json::nullValue;
            Json::Value result = this->client->CallMethod("eth_coinbase",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        std::string eth_compile(const std::string& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_compile",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        double eth_countAt(const std::string& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_countAt",p);
    if (result.isDouble())
        return result.asDouble();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        int eth_defaultBlock() throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p = Json::nullValue;
            Json::Value result = this->client->CallMethod("eth_defaultBlock",p);
    if (result.isInt())
        return result.asInt();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        std::string eth_gasPrice() throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p = Json::nullValue;
            Json::Value result = this->client->CallMethod("eth_gasPrice",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        Json::Value eth_getMessages(const int& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_getMessages",p);
    if (result.isArray())
        return result;
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        bool eth_listening() throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p = Json::nullValue;
            Json::Value result = this->client->CallMethod("eth_listening",p);
    if (result.isBool())
        return result.asBool();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        std::string eth_lll(const std::string& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_lll",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        bool eth_mining() throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p = Json::nullValue;
            Json::Value result = this->client->CallMethod("eth_mining",p);
    if (result.isBool())
        return result.asBool();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        int eth_newFilter(const Json::Value& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_newFilter",p);
    if (result.isInt())
        return result.asInt();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        int eth_newFilterString(const std::string& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_newFilterString",p);
    if (result.isInt())
        return result.asInt();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        int eth_number() throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p = Json::nullValue;
            Json::Value result = this->client->CallMethod("eth_number",p);
    if (result.isInt())
        return result.asInt();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        int eth_peerCount() throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p = Json::nullValue;
            Json::Value result = this->client->CallMethod("eth_peerCount",p);
    if (result.isInt())
        return result.asInt();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        bool eth_setCoinbase(const std::string& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_setCoinbase",p);
    if (result.isBool())
        return result.asBool();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        bool eth_setDefaultBlock(const int& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_setDefaultBlock",p);
    if (result.isBool())
        return result.asBool();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        bool eth_setListening(const bool& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_setListening",p);
    if (result.isBool())
        return result.asBool();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        bool eth_setMining(const bool& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_setMining",p);
    if (result.isBool())
        return result.asBool();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        std::string eth_stateAt(const std::string& param1, const std::string& param2) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);
p.append(param2);

            Json::Value result = this->client->CallMethod("eth_stateAt",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        std::string eth_transact(const Json::Value& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_transact",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        Json::Value eth_transactionByHash(const std::string& param1, const int& param2) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);
p.append(param2);

            Json::Value result = this->client->CallMethod("eth_transactionByHash",p);
    if (result.isObject())
        return result;
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        Json::Value eth_transactionByNumber(const int& param1, const int& param2) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);
p.append(param2);

            Json::Value result = this->client->CallMethod("eth_transactionByNumber",p);
    if (result.isObject())
        return result;
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        Json::Value eth_uncleByHash(const std::string& param1, const int& param2) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);
p.append(param2);

            Json::Value result = this->client->CallMethod("eth_uncleByHash",p);
    if (result.isObject())
        return result;
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        Json::Value eth_uncleByNumber(const int& param1, const int& param2) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);
p.append(param2);

            Json::Value result = this->client->CallMethod("eth_uncleByNumber",p);
    if (result.isObject())
        return result;
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        bool eth_uninstallFilter(const int& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("eth_uninstallFilter",p);
    if (result.isBool())
        return result.asBool();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        std::string shh_addToGroup(const std::string& param1, const std::string& param2) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);
p.append(param2);

            Json::Value result = this->client->CallMethod("shh_addToGroup",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        Json::Value shh_changed(const int& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("shh_changed",p);
    if (result.isArray())
        return result;
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        bool shh_haveIdentity(const std::string& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("shh_haveIdentity",p);
    if (result.isBool())
        return result.asBool();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        int shh_newFilter(const Json::Value& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("shh_newFilter",p);
    if (result.isInt())
        return result.asInt();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        std::string shh_newGroup(const std::string& param1, const std::string& param2) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);
p.append(param2);

            Json::Value result = this->client->CallMethod("shh_newGroup",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        std::string shh_newIdentity() throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p = Json::nullValue;
            Json::Value result = this->client->CallMethod("shh_newIdentity",p);
    if (result.isString())
        return result.asString();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        bool shh_post(const Json::Value& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("shh_post",p);
    if (result.isBool())
        return result.asBool();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

        bool shh_uninstallFilter(const int& param1) throw (jsonrpc::JsonRpcException)
        {
            Json::Value p;
            p.append(param1);

            Json::Value result = this->client->CallMethod("shh_uninstallFilter",p);
    if (result.isBool())
        return result.asBool();
     else 
         throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

        }

    private:
        jsonrpc::Client* client;
};
#endif //_WEBTHREESTUBCLIENT_H_
