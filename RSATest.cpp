#include <cryptopp/rsa.h>
#include <cryptopp/osrng.h>
#include <string>
#include <exception>
#include <iostream>

std::string RSAEncrypt(std::string &input, CryptoPP::RSA::PublicKey pub)
{
	CryptoPP::AutoSeededRandomPool rng;	
	std::string ciphertext;	
	CryptoPP::RSAES_OAEP_SHA_Encryptor e( pub );
	CryptoPP::StringSource ss1( input, true, new CryptoPP::PK_EncryptorFilter( rng, e, new CryptoPP::StringSink( ciphertext )));
	return ciphertext;
}

std::string RSADecrypt(std::string &input, CryptoPP::RSA::PrivateKey pri)
{
	CryptoPP::AutoSeededRandomPool rng;
	std::string recovered;
	CryptoPP::RSAES_OAEP_SHA_Decryptor d( pri );
	CryptoPP::StringSource ss2( input, true, new CryptoPP::PK_DecryptorFilter( rng, d, new CryptoPP::StringSink( recovered )) ); 
	return recovered;
}

int main(int argc, char* argv[])
{
	//Don't send messages larger than 1023 bits
	CryptoPP::AutoSeededRandomPool rng;	//Create a random pool
	CryptoPP::InvertibleRSAFunction params; //Create a public/private key struct
	params.GenerateRandomWithKeySize(rng, 1024); //Generate random public/private key pair
	CryptoPP::RSA::PrivateKey privateKey(params); //Define private key
	CryptoPP::RSA::PublicKey publicKey(params); //Define public key
	std::string plaintext = "transfer[987][123]";
	std::string ciphertext = RSAEncrypt(plaintext, publicKey);
	std::cout<<"Plaintext: "<<plaintext<<std::endl;
	std::cout<<"Ciphertext: "<<ciphertext<<std::endl;
	std::cout<<"Recovered: "<<RSADecrypt(ciphertext, privateKey)<<std::endl;
}
