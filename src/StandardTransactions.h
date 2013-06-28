////////////////////////////////////////////////////////////////////////////////
//
// StandardTransactions.h
//
// Copyright (c) 2011-2012 Eric Lombrozo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef STANDARD_TRANSACTIONS_H__
#define STANDARD_TRANSACTIONS_H__

#include "CoinNodeData.h"
#include "Base58Check.h"
#include "hash.h"

#include <map>
#include <sstream>

const unsigned char BITCOIN_ADDRESS_VERSIONS[] = {0x00, 0x05};

using namespace Coin;

class StandardTxOut : public TxOut
{
public:
    void set(const std::string& address, uint64_t value, const unsigned char addressVersions[] = BITCOIN_ADDRESS_VERSIONS);
};


void StandardTxOut::set(const std::string& address, uint64_t value, const unsigned char addressVersions[])
{
    uchar_vector pubKeyHash;
    uint version;
    if (!fromBase58Check(address, pubKeyHash, version))
        throw std::runtime_error("Invalid address checksum.");

    if (version == addressVersions[0]) {
        // pay-to-address
        this->scriptPubKey = uchar_vector("76a914") + pubKeyHash + uchar_vector("88ac");
    }
    else if (version == addressVersions[1]) {
        // pay-to-script-hash
        this->scriptPubKey = uchar_vector("a914") + pubKeyHash + uchar_vector("87");
    }
    else {
        throw std::runtime_error("Invalid address version.");
    }

    if (pubKeyHash.size() != 20) {
        throw std::runtime_error("Invalid hash length.");
    }

    this->value = value;
}

enum ScriptSigType { SCRIPT_SIG_BROADCAST, SCRIPT_SIG_EDIT, SCRIPT_SIG_SIGN };
enum SigHashType {
    SIGHASH_ALL             = 0x01,
    SIGHASH_NONE            = 0x02,
    SIGHASH_SINGLE          = 0x03,
    SIGHASH_ANYONECANPAY    = 0x80
};

class StandardTxIn : public TxIn
{
public:
    StandardTxIn() { }
    StandardTxIn(const uchar_vector& _outhash, uint32_t _outindex, uint32_t _sequence) :
        TxIn(OutPoint(_outhash, _outindex), "", _sequence) { }

    virtual void clearPubKeys() = 0;
    virtual void addPubKey(const uchar_vector& _pubKey) = 0;

    virtual void clearSigs() = 0;
    virtual void addSig(const uchar_vector& _pubKey, const uchar_vector& _sig, SigHashType sigHashType) = 0;

    virtual void setScriptSig(ScriptSigType scriptSigType) = 0;
};

class P2AddressTxIn : public StandardTxIn
{
private:
    uchar_vector pubKey;
    uchar_vector sig;

public:
    P2AddressTxIn() : StandardTxIn() { }
    P2AddressTxIn(const uchar_vector& _outhash, uint32_t _outindex, const uchar_vector& _pubKey = uchar_vector(), uint32_t _sequence = 0xffffffff) :
        StandardTxIn(_outhash, _outindex, _sequence), pubKey(_pubKey) { }

    void clearPubKeys() { pubKey.clear(); }
    void addPubKey(const uchar_vector& _pubKey);

    void clearSigs() { this->sig = ""; }
    void addSig(const uchar_vector& _pubKey, const uchar_vector& _sig, SigHashType sigHashType = SIGHASH_ALL);

    void setScriptSig(ScriptSigType scriptSigType);
};

void P2AddressTxIn::addPubKey(const uchar_vector& _pubKey)
{
    if (pubKey.size() > 0) {
        throw std::runtime_error("PubKey already added.");
    }
    pubKey = _pubKey;
}

void P2AddressTxIn::addSig(const uchar_vector& _pubKey, const uchar_vector& _sig, SigHashType sigHashType)
{
    if (pubKey.size() == 0) {
        throw std::runtime_error("No PubKey added yet.");
    }

    if (_pubKey != pubKey) {
        throw std::runtime_error("PubKey not part of input.");
    }

    sig = _sig;
    sig.push_back(sigHashType);
}

void P2AddressTxIn::setScriptSig(ScriptSigType scriptSigType)
{
    scriptSig.clear();

    if (scriptSigType == SCRIPT_SIG_SIGN) {
        scriptSig.push_back(0x76);
        scriptSig.push_back(0xa9);
        scriptSig.push_back(0x14);
        scriptSig += ripemd160(sha256(pubKey));
        scriptSig.push_back(0x88);
        scriptSig.push_back(0xac);
    }
    else {
        scriptSig.push_back(sig.size()); // includes SigHashType
        scriptSig += sig;

        scriptSig.push_back(pubKey.size());
        scriptSig += pubKey;
    }
}


class MultiSigRedeemScript
{
private:
    uint minSigs;
    std::vector<uchar_vector> pubKeys;

    const unsigned char* addressVersions;
    const char* base58chars;

    mutable uchar_vector redeemScript;
    mutable bool bUpdated;

public:
    MultiSigRedeemScript(const uchar_vector& redeemScript) { parseRedeemScript(redeemScript); }
    MultiSigRedeemScript(uint minSigs = 1,
                         const unsigned char* _addressVersions = BITCOIN_ADDRESS_VERSIONS,
                         const char* _base58chars = BITCOIN_BASE58_CHARS) :
        addressVersions(_addressVersions), base58chars(_base58chars), bUpdated(false) { this->setMinSigs(minSigs); }

    void setMinSigs(uint minSigs);
    uint getMinSigs() const { return minSigs; }

    void setAddressTypes(const unsigned char* addressVersions, const char* base58chars = BITCOIN_BASE58_CHARS)
    {
        this->addressVersions = addressVersions;
        this->base58chars = base58chars;
    }

    void clearPubKeys() { pubKeys.clear(); this->bUpdated = false; }
    void addPubKey(const uchar_vector& pubKey);
    uint getPubKeyCount() const { return pubKeys.size(); }
    std::vector<uchar_vector> getPubKeys() const { return pubKeys; }

    void parseRedeemScript(const uchar_vector& redeemScript);
    uchar_vector getRedeemScript() const;
    std::string getAddress() const;

    std::string toJson(bool bShowPubKeys = false) const;
};

void MultiSigRedeemScript::setMinSigs(uint minSigs)
{
    if (minSigs < 1) {
        throw std::runtime_error("At least one signature is required.");
    }

    if (minSigs > 16) {
        throw std::runtime_error("At most 16 signatures are allowed.");
    }

    this->minSigs = minSigs;
    this->bUpdated = false;
}

void MultiSigRedeemScript::addPubKey(const uchar_vector& pubKey)
{
    if (pubKeys.size() >= 16) {
        throw std::runtime_error("Public key maximum of 16 already reached.");
    }

    if (pubKey.size() > 75) {
        throw std::runtime_error("Public keys can be a maximum of 75 bytes.");
    }

    pubKeys.push_back(pubKey);
    bUpdated = false;
}

void MultiSigRedeemScript::parseRedeemScript(const uchar_vector& redeemScript)
{
    if (redeemScript.size() < 3) {
        throw std::runtime_error("Redeem script is too short.");
    }

    // OP_1 is 0x51, OP_16 is 0x60
    unsigned char mSigs = redeemScript[0];
    if (mSigs < 0x51 || mSigs > 0x60) {
        throw std::runtime_error("Invalid signature minimum.");
    }

    unsigned char nKeys = 0x50;
    uint i = 1;
    std::vector<uchar_vector> _pubKeys;
    while (true) {
        unsigned char byte = redeemScript[i++];
        if (i >= redeemScript.size()) {
            throw std::runtime_error("Script terminates prematurely.");
        }
        if ((byte >= 0x51) && (byte <= 0x60)) {
            // interpret byte as the signature counter.
            if (byte != nKeys) {
                throw std::runtime_error("Invalid signature count.");
            }
            if (nKeys < mSigs) {
                throw std::runtime_error("The required signature minimum exceeds the number of keys.");
            }
            if (redeemScript[i++] != 0xae || i > redeemScript.size()) {
                throw std::runtime_error("Invalid script termination.");
            }
            break;
        }
        // interpret byte as the pub key size
        if ((byte > 0x4b) || (i + byte > redeemScript.size())) {
            std::stringstream ss;
            ss << "Invalid OP at byte " << i - 1 << ".";
            throw std::runtime_error(ss.str());
        }
        nKeys++;
        if (nKeys > 0x60) {
            throw std::runtime_error("Public key maximum of 16 exceeded.");
        }
        _pubKeys.push_back(uchar_vector(redeemScript.begin() + i, redeemScript.begin() + i + byte));
        i += byte;
    }

    minSigs = mSigs - 0x50;
    pubKeys = _pubKeys;
}

uchar_vector MultiSigRedeemScript::getRedeemScript() const
{
    if (!bUpdated) {
        uint nKeys = pubKeys.size();

        if (minSigs > nKeys) {
            throw std::runtime_error("Insufficient public keys.");
        }

        redeemScript.clear();
        redeemScript.push_back((unsigned char)(minSigs + 0x50));
        for (uint i = 0; i < nKeys; i++) {
            redeemScript.push_back(pubKeys[i].size());
            redeemScript += pubKeys[i];
        }
        redeemScript.push_back((unsigned char)(nKeys + 0x50));
        redeemScript.push_back(0xae); // OP_CHECKMULTISIG
        bUpdated = true;
    }

    return redeemScript;
}

std::string MultiSigRedeemScript::getAddress() const
{
    uchar_vector scriptHash = ripemd160(sha256(getRedeemScript()));
    return toBase58Check(scriptHash, addressVersions[1], base58chars);
}

std::string MultiSigRedeemScript::toJson(bool bShowPubKeys) const
{
    uint nKeys = pubKeys.size();
    std::stringstream ss;
    ss <<   "{\n    \"m\" : " << minSigs
       <<   ",\n    \"n\" : " << nKeys
       <<   ",\n    \"address\" : \"" << getAddress()
       << "\",\n    \"redeemScript\" : \"" << getRedeemScript().getHex() << "\"";
    if (bShowPubKeys) {
        ss << ",\n    \"pubKeys\" :\n    [";
        for (uint i = 0; i < nKeys; i++) {
            uchar_vector pubKeyHash = ripemd160(sha256(pubKeys[i]));
            std::string address = toBase58Check(pubKeyHash, addressVersions[0], base58chars);
            if (i > 0) ss << ",";
            ss <<    "\n        {"
               <<    "\n            \"address\" : \"" << address
               << "\",\n            \"pubKey\" : \"" << pubKeys[i].getHex()
               <<  "\"\n        }";
        }
        ss << "\n    ]";
    }
    ss << "\n}";
    return ss.str();
}


class MofNTxIn : public StandardTxIn
{
private:
    uint minSigs;

    std::map<uchar_vector, uchar_vector> mapPubKeyToSig;
    std::vector<uchar_vector> pubKeys;

public:
    MofNTxIn() : StandardTxIn() { minSigs = 0; }
    MofNTxIn(const uchar_vector& outhash, uint32_t outindex, const MultiSigRedeemScript& redeemScript = MultiSigRedeemScript(), uint32_t sequence = 0xffffffff) :
        StandardTxIn(outhash, outindex, sequence) { setRedeemScript(redeemScript); }

    void setRedeemScript(const MultiSigRedeemScript& redeemScript);

    void clearPubKeys() { mapPubKeyToSig.clear(); pubKeys.clear(); }
    void addPubKey(const uchar_vector& pubKey);

    void clearSigs();
    void addSig(const uchar_vector& pubKey, const uchar_vector& sig, SigHashType sigHashType = SIGHASH_ALL);

    void setScriptSig(ScriptSigType scriptSigType);
};

void MofNTxIn::setRedeemScript(const MultiSigRedeemScript& redeemScript)
{
    minSigs = redeemScript.getMinSigs();
    pubKeys = redeemScript.getPubKeys();

    mapPubKeyToSig.clear();
    for (uint i = 0; i < pubKeys.size(); i++) {
        mapPubKeyToSig[pubKeys[i]] = uchar_vector();
    }
}

void MofNTxIn::addPubKey(const uchar_vector& pubKey)
{
    std::map<uchar_vector, uchar_vector>::iterator it;
    it = mapPubKeyToSig.find(pubKey);
    if (it != mapPubKeyToSig.end()) {
        throw std::runtime_error("PubKey already added.");
    }

    mapPubKeyToSig[pubKey] = uchar_vector();
}

// TODO: verify it->second can be set
void MofNTxIn::clearSigs()
{
    std::map<uchar_vector, uchar_vector>::iterator it = mapPubKeyToSig.begin();
    for(; it != mapPubKeyToSig.end(); ++it) {
        it->second = uchar_vector();
    }
}

void MofNTxIn::addSig(const uchar_vector& pubKey, const uchar_vector& sig, SigHashType sigHashType)
{
    std::map<uchar_vector, uchar_vector>::iterator it;
    it = mapPubKeyToSig.find(pubKey);
    if (it == mapPubKeyToSig.end()) {
        std::stringstream ss;
        ss << "PubKey " << pubKey.getHex() << " not yet added.";
        throw std::runtime_error(ss.str());
    }

    uchar_vector _sig = sig;
    _sig.push_back(sigHashType);
    mapPubKeyToSig[pubKey] = _sig;
}

void MofNTxIn::setScriptSig(ScriptSigType scriptSigType)
{
    if (scriptSigType == SCRIPT_SIG_BROADCAST || scriptSigType == SCRIPT_SIG_EDIT) {
        scriptSig.clear();
        scriptSig.push_back(0x00); // OP_FALSE

        for (uint i = 0; i < pubKeys.size(); i++) {
            uchar_vector sig = mapPubKeyToSig[pubKeys[i]];
            if (sig.size() > 0 || scriptSigType == SCRIPT_SIG_EDIT) {
                scriptSig.push_back(sig.size());
                scriptSig += sig;
            }
        }
    }

    MultiSigRedeemScript redeemScript(minSigs);
    for (uint i = 0; i < pubKeys.size(); i++) {
        redeemScript.addPubKey(pubKeys[i]);
    }
    
    scriptSig.push_back(redeemScript.getRedeemScript().size());
    scriptSig += redeemScript.getRedeemScript();    
}



class P2SHTxIn : public StandardTxIn
{
private:
    uchar_vector redeemScript;
    
    std::vector<uchar_vector> sigs;

public:
    P2SHTxIn() : StandardTxIn() { }
    P2SHTxIn(const uchar_vector& outhash, uint32_t outindex, const uchar_vector& _redeemScript = uchar_vector(), uint32_t sequence = 0xffffffff) :
        StandardTxIn(outhash, outindex, sequence), redeemScript(_redeemScript) { }

    void setRedeemScript(const uchar_vector& redeemScript) { this->redeemScript = redeemScript; }
    const uchar_vector& getRedeemScript() const { return this->redeemScript; }

    void clearPubKeys() { }
    void addPubKey(const uchar_vector& pubKey) { }

    void clearSigs() { sigs.clear(); }
    void addSig(const uchar_vector& pubKey, const uchar_vector& sig, SigHashType sigHashType = SIGHASH_ALL)
    {
        uchar_vector _sig = sig;
        _sig.push_back(sigHashType);
        sigs.push_back(_sig);
    }

    void setScriptSig(ScriptSigType scriptSigType);
};

void P2SHTxIn::setScriptSig(ScriptSigType scriptSigType)
{
    scriptSig.clear();
    scriptSig.push_back(0x00); // OP_FALSE

    for (uint i = 0; i < sigs.size(); i++) {
        scriptSig.push_back(sigs[i].size());
        scriptSig += sigs[i];
    }

    scriptSig.push_back(redeemScript.size());
    scriptSig += redeemScript;
}



class TransactionBuilder
{
private:
    uint32_t version;
    std::vector<StandardTxIn*> inputs;
    std::vector<StandardTxOut*> outputs;
    uint32_t lockTime;

public:
    TransactionBuilder() { }
    TransactionBuilder(const Transaction& tx) { this->setTx(tx); }
    
    ~TransactionBuilder();

    void clearInputs();
    void clearOutputs();

    void setTx(const Transaction& tx);
};

void TransactionBuilder::setTx(const Transaction& tx)
{
    version = tx.version;
    lockTime = tx.lockTime;
    clearInputs();
    clearOutputs();

    for (uint i = 0; i < tx.inputs.size(); i++) {
        std::vector<uchar_vector> objects;
        uint s = 0;
        while (s < tx.inputs[i].scriptSig.size()) {
            uint start = s + 1;
            uint end = s + tx.inputs[i].scriptSig[s] + 1;
            if (end > tx.inputs[i].scriptSig.size()) {
                std::stringstream ss;
                ss << "Tried to push object that exceeeds scriptSig size in input " << i;
                throw std::runtime_error(ss.str());
            }
            objects.push_back(uchar_vector(tx.inputs[i].scriptSig.begin() + start, tx.inputs[i].scriptSig.begin() + end));
            s = end;
        }

        if (objects.size() == 2) {
            P2AddressTxIn* pTxIn = new P2AddressTxIn();
            inputs.push_back(pTxIn); 
        }
    } 
}

void TransactionBuilder::clearInputs()
{
    for (uint i = 0; i < inputs.size(); i++) {
        delete inputs[i];
    }
    inputs.clear();
}

void TransactionBuilder::clearOutputs()
{
    for (uint i = 0; i < outputs.size(); i++) {
        delete outputs[i];
    }
    outputs.clear();
}

TransactionBuilder::~TransactionBuilder()
{
    clearInputs();
    clearOutputs();
}

#endif // STANDARD_TRANSACTIONS_H__
