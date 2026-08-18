//
// Created by George Urgiles on 8/13/26.
//

#ifndef GAMESERVER_AUDITSIGNER_H
#define GAMESERVER_AUDITSIGNER_H
#include <string>

class AuditSigner {
private:
    std::string secret_key;

public:
    AuditSigner(const std::string& secret_key);
    std::string signPayload(const std::string& payload);

};

#endif //GAMESERVER_AUDITSIGNER_H