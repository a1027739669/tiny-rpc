#include <google/protobuf/service.h>
#include <sstream>
#include <atomic>

#include "tinyrpc/net/tcp/tcp_server.h"
#include "tinyrpc/net/net_address.h"
#include "tinyrpc/net/tinypb/tinypb_rpc_dispatcher.h"
#include "tinyrpc/comm/log.h"
#include "tinyrpc/comm/start.h"
#include "test_tinypb_server.pb.h"

#ifdef DECLARE_MYSQL_PLUGIN

#include <mysql/mysql.h>
#include "tinyrpc/comm/mysql_instase.h"

#endif


class QueryServiceImpl : public QueryService {
 public:
  QueryServiceImpl() {}
  ~QueryServiceImpl() {}

  void query_name(google::protobuf::RpcController* controller,
                       const ::queryNameReq* request,
                       ::queryNameRes* response,
                       ::google::protobuf::Closure* done) {
    
    AppInfoLog << "QueryServiceImpl.query_name, req={"<< request->ShortDebugString() << "}";

    // DebugLog << "========================";
    // DebugLog << "this is query_name func";
    // DebugLog << "first begin to sleep 6s";
    // sleep(6);
    // DebugLog << "sleep 6s end";

    response->set_ret_code(0);
    response->set_res_info("OK");

    #ifdef DECLARE_MYSQL_PLUGIN

    AppDebugLog << "install mysql pulgin, begin to query mysql";
    tinyrpc::MySQLInstase::ptr instase =  tinyrpc::MySQLInstaseFactroy::GetThreadMySQLFactory()->GetMySQLInstase("test_db_key1");
    if (!instase || !instase->isInitSuccess()) {
      response->set_ret_code(-1);
      response->set_res_info("faild to init mysql");
      AppErrorLog << "mysql instase init failed";
      return;
    }

    int n = 1000000;
    // while(n--) {
    char query_sql[512];
    sprintf(query_sql, "select user_id, user_name, user_gender from user_db.t_user_information where user_id = '%s';", std::to_string(request->id()).c_str());

    int rt = instase->query(std::string(query_sql));
    if (rt != 0) {
      response->set_ret_code(-1);
      response->set_res_info(instase->getMySQLErrorInfo());
      AppErrorLog << "failed to excute sql" << std::string(query_sql);
      return;
    }

    MYSQL_RES* res = instase->storeResult();

    MYSQL_ROW row = instase->fetchRow(res);
    if (row) {
      int i = 0;
      AppDebugLog << "query success";
      response->set_id(std::atoi(row[i++]));
      response->set_name(std::string(row[i++]));
    } else {
      AppDebugLog << "query empty";
      response->set_ret_code(-1);
      response->set_res_info("this user not exist");
    }
    instase->freeResult(res);

    #else

    AppDebugLog << "no install mysql pulgin, now direct return default value";
    response->set_id(request->id());
    response->set_name("ikerli");

    #endif

    AppInfoLog << "QueryServiceImpl.query_name, req={"<< request->ShortDebugString() << "}, res={" << response->ShortDebugString() << "}";

    if (done) {
      done->Run();
    }

  }

  void query_age(google::protobuf::RpcController* controller,
                       const ::queryAgeReq* request,
                       ::queryAgeRes* response,
                       ::google::protobuf::Closure* done) {

    AppInfoLog << "QueryServiceImpl.query_age, req={"<< request->ShortDebugString() << "}";
    // AppInfoLog << "QueryServiceImpl.query_age, sleep 6 s begin";
    // sleep(6);
    // AppInfoLog << "QueryServiceImpl.query_age, sleep 6 s end";

    response->set_ret_code(0);
    response->set_res_info("OK");
    response->set_req_no(request->req_no());
    response->set_id(request->id());
    response->set_age(100100111);

    if (done) {
      done->Run();
    }
    // printf("response = %s\n", response->ShortDebugString().c_str());

    AppInfoLog << "QueryServiceImpl.query_age, res={"<< response->ShortDebugString() << "}";

  }

};


int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Start TinyRPC server error, input argc is not 2!");
    printf("Start TinyRPC server like this: \n");
    printf("./server a.xml\n");
    return 0;
  }

  tinyrpc::InitConfig(argv[1]);

  REGISTER_SERVICE(QueryServiceImpl);

  tinyrpc::StartRpcServer();
  
  return 0;
}
