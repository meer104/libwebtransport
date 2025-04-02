#ifndef WEB_TRANSPORT_CLIENT_VERIFY_H
#define WEB_TRANSPORT_CLIENT_VERIFY_H

#include <openssl/x509.h>
#include <string>
#include <vector>
#include <memory>
#include <string>
#include <vector>
#include <memory>
#include "quiche/quic/core/crypto/proof_verifier.h"

// The WebTransport namespace.
namespace webtransport
{

    // BoringSSLProofVerifier implements a QUIC ProofVerifier interface.
    // It supports two modes:
    //  1. Pinned certificate mode (if public_key_file is provided).
    //  2. CA chain verification mode (using a CA bundle or directory, or default store).
    class BoringSSLProofVerifier : public quic::ProofVerifier
    {
    public:
        // Default constructor.
        BoringSSLProofVerifier();

        // Custom constructor:
        //  - public_key_file: if non-empty, use the pinned certificate mode.
        //  - ca_cert_bundle_path and ca_cert_dir: used if public_key_file is empty.
        BoringSSLProofVerifier(const std::string &public_key_file,
                               const std::string &ca_cert_bundle_path,
                               const std::string &ca_cert_dir);

        // Override of ProofVerifier::VerifyProof.
        quic::QuicAsyncStatus VerifyProof(
            const std::string &hostname, const uint16_t port,
            const std::string &server_config, quic::QuicTransportVersion transport_version,
            absl::string_view chlo_hash, const std::vector<std::string> &certs,
            const std::string &cert_sct, const std::string &signature,
            const quic::ProofVerifyContext *context, std::string *error_details,
            std::unique_ptr<quic::ProofVerifyDetails> *details,
            std::unique_ptr<quic::ProofVerifierCallback> callback) override;

        // Override of ProofVerifier::VerifyCertChain.
        quic::QuicAsyncStatus VerifyCertChain(
            const std::string &hostname, const uint16_t port,
            const std::vector<std::string> &certs, const std::string &ocsp_response,
            const std::string &cert_sct, const quic::ProofVerifyContext *context,
            std::string *error_details, std::unique_ptr<quic::ProofVerifyDetails> *details,
            uint8_t *out_alert, std::unique_ptr<quic::ProofVerifierCallback> callback) override;

        // Override of ProofVerifier::CreateDefaultContext.
        std::unique_ptr<quic::ProofVerifyContext> CreateDefaultContext() override;

    private:
        std::string public_key_file_;
        std::string ca_cert_bundle_path_;
        std::string ca_cert_dir_;
        bool use_public_key_;
    };

    // Helper function declarations.
    // These functions load certificate data from a string and file, respectively.
    X509 *LoadCertificate(const std::string &cert_str);
    bool LoadFile(const std::string &filename, std::string &contents);

} // namespace webtransport

#endif // WEB_TRANSPORT_CLIENT_VERIFY_H