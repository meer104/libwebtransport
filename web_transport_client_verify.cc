#include "web_transport_client_verify.h"

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>
#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>

// Anonymous namespace for helper functions.
namespace
{

  // Helper to load a certificate from a string (supports PEM or DER).
  X509 *LoadCertificate(const std::string &cert_str)
  {
    X509 *cert = nullptr;
    if (cert_str.find("-----BEGIN CERTIFICATE-----") != std::string::npos)
    {
      BIO *bio = BIO_new_mem_buf(cert_str.data(), static_cast<int>(cert_str.size()));
      if (!bio)
        return nullptr;
      cert = PEM_read_bio_X509(bio, nullptr, 0, nullptr);
      BIO_free(bio);
    }
    else
    {
      const unsigned char *p = reinterpret_cast<const unsigned char *>(cert_str.data());
      cert = d2i_X509(nullptr, &p, cert_str.size());
    }
    return cert;
  }

  // Helper to load a file's entire contents into a string.
  bool LoadFile(const std::string &filename, std::string &contents)
  {
    std::ifstream file(filename);
    if (!file)
      return false;
    std::stringstream ss;
    ss << file.rdbuf();
    contents = ss.str();
    return true;
  }

} // namespace

namespace webtransport
{

  BoringSSLProofVerifier::BoringSSLProofVerifier()
      : public_key_file_(""),
        ca_cert_bundle_path_(""),
        ca_cert_dir_(""),
        use_public_key_(false) {}

  BoringSSLProofVerifier::BoringSSLProofVerifier(
      const std::string &public_key_file,
      const std::string &ca_cert_bundle_path,
      const std::string &ca_cert_dir)
      : public_key_file_(public_key_file),
        ca_cert_bundle_path_(ca_cert_bundle_path),
        ca_cert_dir_(ca_cert_dir),
        use_public_key_(!public_key_file.empty()) {}

  // This method is a stub that simply prints a message and returns success.
  quic::QuicAsyncStatus BoringSSLProofVerifier::VerifyProof(
      const std::string &hostname, const uint16_t port,
      const std::string &server_config, quic::QuicTransportVersion transport_version,
      absl::string_view chlo_hash, const std::vector<std::string> &certs,
      const std::string &cert_sct, const std::string &signature,
      const quic::ProofVerifyContext *context, std::string *error_details,
      std::unique_ptr<quic::ProofVerifyDetails> *details,
      std::unique_ptr<quic::ProofVerifierCallback> callback)
  {
    std::cerr << "[+] Checking VerifyProof instead" << std::endl;
    return quic::QUIC_SUCCESS;
  }

  quic::QuicAsyncStatus BoringSSLProofVerifier::VerifyCertChain(
      const std::string &hostname, const uint16_t port,
      const std::vector<std::string> &certs, const std::string &ocsp_response,
      const std::string &cert_sct, const quic::ProofVerifyContext *context,
      std::string *error_details, std::unique_ptr<quic::ProofVerifyDetails> *details,
      uint8_t *out_alert, std::unique_ptr<quic::ProofVerifierCallback> callback)
  {
    if (certs.empty())
    {
      std::cerr << "No certificates provided." << std::endl;
      return quic::QUIC_FAILURE;
    }

    std::vector<X509 *> x509_chain;
    for (const auto &cert_str : certs)
    {
      X509 *cert = LoadCertificate(cert_str);
      if (!cert)
      {
        std::cerr << "Failed to load a certificate from provided data." << std::endl;
        for (auto c : x509_chain)
        {
          X509_free(c);
        }
        return quic::QUIC_FAILURE;
      }
      x509_chain.push_back(cert);
    }

    bool valid = false;

    if (use_public_key_)
    {
      // Pinned certificate verification.
      if (x509_chain.size() == 1)
      {
        std::string pinned_cert_data;
        if (!LoadFile(public_key_file_, pinned_cert_data))
        {
          std::cerr << "Failed to load pinned certificate from file: "
                    << public_key_file_ << std::endl;
        }
        else
        {
          X509 *pinned_cert = LoadCertificate(pinned_cert_data);
          if (!pinned_cert)
          {
            std::cerr << "Failed to parse pinned certificate." << std::endl;
          }
          else
          {
            if (X509_cmp(x509_chain[0], pinned_cert) == 0)
            {
              std::cerr << "Pinned certificate verification OK!" << std::endl;
              valid = true;
            }
            else
            {
              std::cerr << "Pinned certificate verification failed." << std::endl;
            }
            X509_free(pinned_cert);
          }
        }
      }
      else
      {
        std::cerr << "Public key verification supports only self-signed certificates (chain size == 1)." << std::endl;
      }
    }
    else
    {
      // CA chain verification.
      X509_STORE *store = X509_STORE_new();
      if (!store)
      {
        std::cerr << "Failed to create X509_STORE." << std::endl;
        for (auto c : x509_chain)
        {
          X509_free(c);
        }
        return quic::QUIC_FAILURE;
      }

      // Use provided CA bundle/directory if available, else use default paths.
      if (!ca_cert_bundle_path_.empty() || !ca_cert_dir_.empty())
      {
        if (X509_STORE_load_locations(
                store,
                ca_cert_bundle_path_.empty() ? nullptr : ca_cert_bundle_path_.c_str(),
                ca_cert_dir_.empty() ? nullptr : ca_cert_dir_.c_str()) != 1)
        {
          std::cerr << "Failed to load CA certificates from specified locations." << std::endl;
          X509_STORE_free(store);
          for (auto c : x509_chain)
          {
            X509_free(c);
          }
          return quic::QUIC_FAILURE;
        }
      }
      else
      {
        if (X509_STORE_set_default_paths(store) != 1)
        {
          std::cerr << "Failed to set default paths on X509_STORE." << std::endl;
          X509_STORE_free(store);
          for (auto c : x509_chain)
          {
            X509_free(c);
          }
          return quic::QUIC_FAILURE;
        }
      }

      X509_STORE_CTX *ctx = X509_STORE_CTX_new();
      if (!ctx)
      {
        std::cerr << "Failed to create X509_STORE_CTX." << std::endl;
        X509_STORE_free(store);
        for (auto c : x509_chain)
        {
          X509_free(c);
        }
        return quic::QUIC_FAILURE;
      }

      STACK_OF(X509) *untrusted = nullptr;
      if (x509_chain.size() > 1)
      {
        STACK_OF(X509) *intermediates = sk_X509_new_null();
        for (size_t i = 1; i < x509_chain.size(); ++i)
        {
          sk_X509_push(intermediates, x509_chain[i]);
        }
        untrusted = intermediates;
      }

      if (X509_STORE_CTX_init(ctx, store, x509_chain[0], untrusted) != 1)
      {
        std::cerr << "Failed to initialize X509_STORE_CTX." << std::endl;
        X509_STORE_CTX_free(ctx);
        X509_STORE_free(store);
        if (untrusted)
          sk_X509_free(untrusted);
        for (auto c : x509_chain)
        {
          X509_free(c);
        }
        return quic::QUIC_FAILURE;
      }

      if (X509_verify_cert(ctx) == 1)
      {
        if (X509_check_host(x509_chain[0], hostname.c_str(), hostname.size(), 0, nullptr) == 1)
        {
          std::cerr << "Hostname verification OK!" << std::endl;
          valid = true;
        }
        else
        {
          std::cerr << "Hostname verification failed." << std::endl;
        }
      }
      else
      {
        int err = X509_STORE_CTX_get_error(ctx);
        std::cerr << "Certificate chain verification failed: "
                  << X509_verify_cert_error_string(err) << std::endl;
      }

      X509_STORE_CTX_free(ctx);
      X509_STORE_free(store);
      if (untrusted)
        sk_X509_free(untrusted);
    }

    // Free the certificates loaded from the chain.
    for (auto cert : x509_chain)
    {
      X509_free(cert);
    }

    return valid ? quic::QUIC_SUCCESS : quic::QUIC_FAILURE;
  }

  std::unique_ptr<quic::ProofVerifyContext> BoringSSLProofVerifier::CreateDefaultContext()
  {
    return nullptr;
  }

  // Expose the helper functions through the header.
  X509 *LoadCertificate(const std::string &cert_str)
  {
    return ::LoadCertificate(cert_str);
  }

  bool LoadFile(const std::string &filename, std::string &contents)
  {
    return ::LoadFile(filename, contents);
  }

} // namespace webtransport