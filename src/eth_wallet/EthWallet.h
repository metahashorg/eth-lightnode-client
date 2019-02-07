#ifndef ETHWALLET_H
#define ETHWALLET_H

#include <string>
#include <vector>

class EthWallet {
public:

    EthWallet(
        const std::string &folder,
        const std::string &address,
        std::string password
    );

    ~EthWallet() {};

    std::string SignTransaction(
        std::string nonce,
        std::string gasPrice,
        std::string gasLimit,
        std::string to,
        std::string value,
        std::string data
    );

    const std::string& getAddress() const;

    static std::string calcHash(const std::string &txHex);

    static void checkAddress(const std::string &address);

    static void baseCheckAddress(const std::string &address);

    static std::string getFullPath(const std::string &folder, const std::string &address);

    static std::string genPrivateKey(const std::string &folder, const std::string &password, bool isLight);

    static std::vector<std::pair<std::string, std::string>> getAllWalletsInFolder(const std::string &folder);

    static std::string makeErc20Data(const std::string &valueHex, const std::string &address);

private:

    EthWallet(const std::string &fileData, const std::string &address, const std::string &password, bool tmp);

private:

    std::vector<uint8_t> rawprivkey;

    std::string address;

};

#endif // ETHWALLET_H
