#include "cert.h"
#include "utils2.h"
#include "const.h"

#include "scrypt/libscrypt.h"

#include <cryptopp/oids.h>
#include <cryptopp/keccak.h>
#include <cryptopp/aes.h>
#include <cryptopp/ccm.h>

#include <iostream>

#include "eth_wallet/jsonUtils.h"

#include "eth_wallet/check.h"
#include "eth_wallet/TypedException.h"

static CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey LoadPrivateKey(uint8_t* privkey, size_t privkeysize)
{
    CryptoPP::Integer x(privkey, privkeysize);
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;
    privateKey.Initialize(CryptoPP::ASN1::secp256k1(), x);
    return privateKey;
}

static void ParseCert(const char* certContent, CertParams& params) {
    rapidjson::Document root;
    const rapidjson::ParseResult pr = root.Parse(certContent);
    CHECK(pr, "rapidjson parse error. Data: " + std::string(certContent));

    CHECK_TYPED(root.HasMember("address") && root["address"].IsString(), TypeErrors::PRIVATE_KEY_ERROR, "address field not found in private key");
    params.address = "0x" + std::string(root["address"].GetString());
    CHECK_TYPED(root.HasMember("version") && root["version"].IsInt(), TypeErrors::PRIVATE_KEY_ERROR, "version field not found in private key");
    params.version = root["version"].GetInt();
    CHECK_TYPED(root.HasMember("crypto") && root["crypto"].IsObject(), TypeErrors::PRIVATE_KEY_ERROR, "crypto field not found in private key");
    const auto &doc = root["crypto"];
    CHECK_TYPED(doc.HasMember("cipher") && doc["cipher"].IsString(), TypeErrors::PRIVATE_KEY_ERROR, "cipher field not found in private key");
    CHECK_TYPED(doc.HasMember("ciphertext") && doc["ciphertext"].IsString(), TypeErrors::PRIVATE_KEY_ERROR, "ciphertext field not found in private key");
    CHECK_TYPED(doc.HasMember("cipherparams") && doc["cipherparams"].IsObject(), TypeErrors::PRIVATE_KEY_ERROR, "cipherparams field not found in private key");
    const auto &doc2 = doc["cipherparams"];
    CHECK_TYPED(doc2.HasMember("iv") && doc2["iv"].IsString(), TypeErrors::PRIVATE_KEY_ERROR, "iv field not found in private key");
    //Читаем параметры для aes
    params.cipher = doc["cipher"].GetString();
    params.ciphertext = doc["ciphertext"].GetString();
    params.iv = doc2["iv"].GetString();

    CHECK_TYPED(doc.HasMember("kdf") && doc["kdf"].IsString(), TypeErrors::PRIVATE_KEY_ERROR, "kdf field not found in private key");
    CHECK_TYPED(doc.HasMember("kdfparams") && doc["kdfparams"].IsObject(), TypeErrors::PRIVATE_KEY_ERROR, "kdfparams field not found in private key");
    const auto &doc3 = doc["kdfparams"];
    CHECK_TYPED(doc3.HasMember("dklen") && doc3["dklen"].IsInt(), TypeErrors::PRIVATE_KEY_ERROR, "dklen field not found in private key");
    CHECK_TYPED(doc3.HasMember("n") && doc3["n"].IsInt(), TypeErrors::PRIVATE_KEY_ERROR, "n field not found in private key");
    CHECK_TYPED(doc3.HasMember("p") && doc3["p"].IsInt(), TypeErrors::PRIVATE_KEY_ERROR, "p field not found in private key");
    CHECK_TYPED(doc3.HasMember("r") && doc3["r"].IsInt(), TypeErrors::PRIVATE_KEY_ERROR, "r field not found in private key");
    CHECK_TYPED(doc3.HasMember("salt") && doc3["salt"].IsString(), TypeErrors::PRIVATE_KEY_ERROR, "salt field not found in private key");
    //Читаем параметры для kdf
    params.kdftype = doc["kdf"].GetString();
    params.dklen = doc3["dklen"].GetInt();
    params.n = doc3["n"].GetInt();
    params.p = doc3["p"].GetInt();
    params.r = doc3["r"].GetInt();
    params.salt = doc3["salt"].GetString();
    CHECK_TYPED(doc.HasMember("mac") && doc["mac"].IsString(), TypeErrors::PRIVATE_KEY_ERROR, "mac field not found in private key");
    params.mac = doc["mac"].GetString();
}

std::string DeriveAESKeyFromPassword(const std::string& password, CertParams& params) {
    uint8_t derivedKey[EC_KEY_LENGTH] = {0};
    std::string rawsalt = HexStringToDump(params.salt);
    const int result = libscrypt_scrypt((const uint8_t*)password.c_str(), password.size(),
                        (const uint8_t*)rawsalt.c_str(), rawsalt.size(),
                        params.n, params.r, params.p,
                        derivedKey, EC_KEY_LENGTH);
    CHECK_TYPED(result == 0, TypeErrors::INCORRECT_PASSWORD, "Incorrect password");
    return std::string((char*)derivedKey, EC_KEY_LENGTH);
}

bool CheckPassword(const std::string& derivedKey, const CertParams& params) {
    uint8_t hs[EC_KEY_LENGTH];
    CHECK_TYPED(derivedKey.size() >= 32, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect derivedKey");
    std::string hashdata = derivedKey.substr(16, 16) + HexStringToDump(params.ciphertext);
    CryptoPP::Keccak k(EC_KEY_LENGTH);
    k.Update((uint8_t*)hashdata.c_str(), hashdata.size());
    k.TruncatedFinal(hs, EC_KEY_LENGTH);
    return (params.mac.compare(DumpToHexString(hs, EC_KEY_LENGTH)) == 0);
}

std::string DecodePrivateKey(const std::string& derivedkey, CertParams& params) {
    std::string privkey = "";
    std::string rawiv = HexStringToDump(params.iv);
    privkey.reserve(EC_PUB_KEY_LENGTH + 10);
    CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption d;
    d.SetKeyWithIV((const uint8_t*)derivedkey.c_str(), 16, (const uint8_t*)rawiv.c_str());
    CryptoPP::StringSource s(HexStringToDump(params.ciphertext), true,
        new CryptoPP::StreamTransformationFilter(d,
            new CryptoPP::StringSink(privkey)
        )
    );
    return privkey;
}

CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey DecodeCert(const char* certContent, const std::string& pass, uint8_t* rawkey)
{
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey pk;
    CertParams params;
    ParseCert(certContent, params);
    std::string derivedkey = DeriveAESKeyFromPassword(pass, params);
    CHECK_TYPED(CheckPassword(derivedkey, params), TypeErrors::INCORRECT_PASSWORD, "incorrect password");
    std::string privkey = DecodePrivateKey(derivedkey, params);
    CHECK_TYPED(privkey.size() >= 32, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect privkey");
    CHECK(EC_KEY_LENGTH >= 32, "Ups");
    memcpy(rawkey, privkey.c_str(), 32);
    pk = LoadPrivateKey((uint8_t*)privkey.c_str(), privkey.size());
    return pk;
}

std::string getAddressFromFile(const char* certContent) {
    CertParams params;
    ParseCert(certContent, params);
    return params.address;
}
