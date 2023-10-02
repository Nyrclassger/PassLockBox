#include <openssl/evp.h>
#include "mainwindow.h"

QByteArray MainWindow::encryptData(const QByteArray &data)
{
    // OpenSSL initialization
    OpenSSL_add_all_algorithms();

    // Key initialization and IV
    unsigned char key[32];
    unsigned char iv[16];
    EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), NULL, (unsigned char*)KEY, strlen(KEY), 1, key, iv);

    // Get the block size of the encryption algorithm
    const EVP_CIPHER* cipher = EVP_aes_256_cbc();
    int block_size = EVP_CIPHER_block_size(cipher);

    // Creating and configuring the encryption context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);
    EVP_EncryptInit_ex(ctx, cipher, NULL, key, iv);

    int out_len; // Variable for storing the length of encrypted data
    QByteArray ciphertext(data.length() + block_size, 0); // Initialize ciphertext buffer

    // Data encryption
    EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()), &out_len, (const unsigned char*)data.data(), data.length());
    int ciphertext_len = out_len;
    EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()) + out_len, &out_len);
    ciphertext_len += out_len;

    EVP_CIPHER_CTX_free(ctx);

    // Resize the ciphertext buffer to the actual length
    ciphertext.resize(ciphertext_len);

    return ciphertext;
}

QByteArray MainWindow::decryptData(const QByteArray &ciphertext)
{
    // OpenSSL initialization
    OpenSSL_add_all_algorithms();

    // Key initialization and IV
    unsigned char key[32];
    unsigned char iv[16];
    EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), NULL, (unsigned char*)KEY, strlen(KEY), 1, key, iv);

    // Creating and customizing the decryption context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    int out_len; // Variable for storing the length of decrypted data
    QByteArray plaintext(ciphertext.length(), 0); // Initialize plaintext buffer

    // Data decryption
    EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plaintext.data()), &out_len, (const unsigned char*)ciphertext.data(), ciphertext.length());
    int plaintext_len = out_len;
    EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plaintext.data()) + out_len, &out_len);
    plaintext_len += out_len;

    EVP_CIPHER_CTX_free(ctx);

    // Resize the plaintext buffer to the actual length
    plaintext.resize(plaintext_len);

    return plaintext;
}
