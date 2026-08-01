# E2E test certificates — TEST MATERIAL ONLY

`ServerPrivateKey.pem` here is a **throwaway test key with no secrecy value**.
It is committed on purpose: the e2e fixtures need deterministic material, and a
key generated per run would make failures irreproducible. Treat it as public,
because it is.

**Never use this keypair in a deployment**, and never copy it into one. The
subject CN says `NOT FOR DEPLOYMENT` so a certificate dumped from a running
server is immediately identifiable as test material.

Deployments generate their own. The GCP demo generates a per-VM keypair into
`/var/lib/scada-certs` on first deploy; the private key never enters git, the
container image, or the config bundle. See
`gcp/free-tier/multitier/make-server-cert.sh` and
`docs/ops/demo-opcua-exposure.md` in the superproject.

## History

Until 2026-08-01 this fixture, the GCP demo, and the local web dev runtime all
shared **one** keypair — self-signed, committed in plaintext, expired since
2021-06-09, and carrying no `applicationUri` SAN (so no conformant client would
accept it, and the demo's advertised `Basic256Sha256` endpoint was meaningless
against anyone with repository access). The three uses were split apart and
regenerated then.

## Regenerating

The generator is in the superproject, so this directory has no build-time
dependency on it — the committed output is all the tests need:

```bash
rm -f ServerCertificate.pem ServerPrivateKey.pem
gcp/free-tier/multitier/make-server-cert.sh \
  common/test/e2e/fixtures/server-data/Certificates \
  --days 3650 --subject-cn "Telecontrol SCADA E2E Test Server (NOT FOR DEPLOYMENT)"
```

Working standalone, without the superproject, any self-signed RSA-2048/SHA-256
certificate works provided its `subjectAltName` carries
`URI:urn:telecontrol:scada:server` (OPC UA Part 6 §6.2.2,
<https://reference.opcfoundation.org/Core/Part6/v105/docs/6.2.2>). The long
validity is deliberate: an expired fixture breaks CI in a way that looks like a
protocol bug, which is exactly what happened before.
