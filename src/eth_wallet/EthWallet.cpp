#include "EthWallet.h"

#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>

#include "ethtx/ethtx.h"
#include "ethtx/utils2.h"
#include "ethtx/const.h"
#include "ethtx/cert.h"

#include "utils.h"
#include "stringUtils.h"
#include "FileSystem.h"
#include "check.h"
#include "TypedException.h"

std::string EthWallet::getFullPath(const std::string &folder, const std::string &address)
{
    std::string addr = address;
    std::transform(addr.begin(), addr.end(), addr.begin(), ::tolower);
    return makePath(folder, addr);
}

EthWallet::EthWallet(const std::string &fileData, const std::string &address, const std::string &password, bool /*tmp*/)
    : address(address)
{
    CHECK_TYPED(!password.empty(), TypeErrors::INCORRECT_USER_DATA, "Empty password");
    const std::string &certcontent = fileData;
    CHECK_TYPED(!certcontent.empty(), TypeErrors::PRIVATE_KEY_ERROR, "private file empty");
    rawprivkey.resize(EC_KEY_LENGTH);
    DecodeCert(certcontent.c_str(), password, rawprivkey.data());
    const std::string calcAddress = "0x" + MixedCaseEncoding(AddressFromPrivateKey(std::string(rawprivkey.begin(), rawprivkey.end())));
    CHECK_TYPED(toLower(calcAddress) == toLower(address), TypeErrors::PRIVATE_KEY_ERROR, "private key error: address calc error");
}

EthWallet::EthWallet(
    const std::string &folder,
    const std::string &address,
    std::string password
)
    : EthWallet(loadFile(getFullPath(folder, address)), address, password, true)
{}

const std::string& EthWallet::getAddress() const {
    return address;
}

std::string EthWallet::SignTransaction(
    std::string nonce,
    std::string gasPrice,
    std::string gasLimit,
    std::string to,
    std::string value,
    std::string data
) {
    baseCheckAddress(to);
    const std::string transaction = ::SignTransaction(std::string((char*)rawprivkey.data(), rawprivkey.size()), nonce, gasPrice, gasLimit, to, value, data);
    return transaction;
}

std::string EthWallet::calcHash(const std::string &txHex) {
    CHECK(txHex.size() > 2 && txHex.compare(0, 2, "0x") == 0, "Incorrect tx");
    return "0x" + toHex(createHashTx(fromHex(txHex.substr(2))));
}

std::string EthWallet::genPrivateKey(const std::string &folder, const std::string &password, bool isLight) {
    CHECK_TYPED(!password.empty(), TypeErrors::INCORRECT_USER_DATA, "Empty password");
    const auto pair = CreateNewKey(password, isLight);
    const std::string &address = pair.first;
    const std::string &keyValue = pair.second;

    CHECK_TYPED(!folder.empty(), TypeErrors::DONT_CREATE_FOLDER, "Incorrect path to wallet: empty");
    const std::string fileName = getFullPath(folder, address);
    EthWallet checkWallet(keyValue, address, password, true);
    CHECK_TYPED(!checkWallet.getAddress().empty(), TypeErrors::PRIVATE_KEY_ERROR, "dont check private key");
    saveToFile(fileName, keyValue);

    return address;
}

std::vector<std::pair<std::string, std::string>> EthWallet::getAllWalletsInFolder(const std::string &folder) {
    std::vector<std::pair<std::string, std::string>> result;

    /*const QDir dir(folder);
    const std::stringList allFiles = dir.entryList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
    for (const std::string &file: allFiles) {
        const std::string fileName = file.toStdString();
        if (fileName.substr(0, 2) == "0x") {
            const std::string addressPart = fileName.substr(2);
            const std::string address = "0x" + MixedCaseEncoding(HexStringToDump(addressPart));
            result.emplace_back(std::string::fromStdString(address), getFullPath(folder, address));
        }
    }*/

    return result;
}

std::string EthWallet::makeErc20Data(const std::string &valueHex, const std::string &address) {
    std::string result = "0xa9059cbb";

    baseCheckAddress(address);
    CHECK_TYPED(valueHex.substr(0, 2) == "0x", TypeErrors::INCORRECT_USER_DATA, "Incorrect address " + valueHex);

    std::string param1 = address.substr(2);
    param1.insert(param1.begin(), 64 - param1.size(), '0');

    std::string param2 = valueHex.substr(2);
    param2.insert(param2.begin(), 64 - param2.size(), '0');

    return result + param1 + param2;
}

void EthWallet::baseCheckAddress(const std::string &address) {
    CHECK_TYPED(address.size() == 42, TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Incorrect address size");
    CHECK_TYPED(address.compare(0, 2, "0x") == 0, TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Incorrect address. 0x missed");
}

void EthWallet::checkAddress(const std::string &address) {
    baseCheckAddress(address);

    const std::string addressPart = address.substr(2);

    const std::string addressMixed = "0x" + MixedCaseEncoding(HexStringToDump(addressPart));
    CHECK_TYPED(addressMixed == address, TypeErrors::INCORRECT_ADDRESS_OR_PUBLIC_KEY, "Incorrect Address");
}
