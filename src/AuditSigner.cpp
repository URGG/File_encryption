//
// Created by George Urgiles on 8/13/26.
//

#include "../AuditSigner.h"


#include <iomanip>
#include <sstream>
#include <openssl/evp.h>

AuditSigner::AuditSigner(const std::string& key) : secret_key(key) {}

std::string AuditSigner::signPayload(const std::string& payload) {

    std::string data_to_hash = payload + secret_key;


    EVP_MD_CTX* context = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;


    EVP_DigestInit_ex(context, md, nullptr);
    EVP_DigestUpdate(context, data_to_hash.c_str(), data_to_hash.length());
    EVP_DigestFinal_ex(context, hash, &lengthOfHash);
    EVP_MD_CTX_free(context);


    std::stringstream ss;
    for(unsigned int i = 0; i < lengthOfHash; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    // Package the final secure payload
    return payload + " | SHA256_SIG: " + ss.str();
}