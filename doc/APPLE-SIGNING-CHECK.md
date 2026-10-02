# Manual Apple signing-secret check

Run **Check Apple signing secrets** in GitHub Actions on the repository's
default branch. The default runner is Apple Silicon; macOS Intel is also
available. This checks configuration before spending time on a product build.

The check exercises all nine repository Actions secrets:

| Settings | Check |
| --- | --- |
| `APPLE_DEVELOPER_ID_CERTIFICATE_BASE64` and `APPLE_DEVELOPER_ID_INSTALLER_CERTIFICATE_BASE64` | Strict single-line Base64 format and decoding, then PKCS12 import |
| Both `APPLE_DEVELOPER_ID_*CERTIFICATE_PASSWORD` values | Import the corresponding certificate and private key |
| `APPLE_DEVELOPER_ID_IDENTITY` | Sign a tiny Mach-O probe with a timestamp and hardened runtime; verify its signature and Application certificate type |
| `APPLE_DEVELOPER_ID_INSTALLER_IDENTITY` | Sign a tiny synthetic PKG; verify its signature and Installer certificate type |
| `APPLE_TEAM_ID` | Match both signing-certificate teams and validate account access |
| `APPLE_ID` and `APPLE_APP_SPECIFIC_PASSWORD` | `notarytool store-credentials --validate` checks the Apple login and team access |

It creates, locks and unlocks a private temporary keychain, then registers it
in the disposable runner's user search list and selects it as the default,
following the established cREXX signing workflow. Certificates,
stored login credentials and synthetic probes are removed afterwards, with
an always-run cleanup step as a fallback. Setting names and stage results
are written to the log and Actions summary. Signing failures also print the
native tool's error with all nine secret values and the temporary keychain
password redacted. Before Application signing it reports the number of valid
code-signing identities and exact name/fingerprint matches for the selector.
Successful PKCS12 import alone does not establish that the
selected signing identity has a usable private key. No product is built or installed,
no installer is submitted for notarization, and no artifact or release is
uploaded. Account validation and signing timestamps require Apple services;
a service outage can fail those stages even when the secrets are correct.

Base64 ending in either `=` or `==` is normal. Copy one continuous string with
no surrounding quotes, extra text or whitespace. An identity must match its
exported certificate; preserve its Common Name's exact spaces. Application
signing also accepts the SHA-1 from `security find-identity -v -p codesigning`.
The `.p12` exports must contain the private keys, not just public certificates.

Other repositories call the reusable z-pdos workflow at a full commit SHA and
pass their own repository secrets. The `check_revision` input must use that
same SHA so the workflow and its cREXX check remain one reviewed revision.
Only the calling repository's default branch can run it. The common workflow
checks out public z-pdos source; it does not read the caller's product or data.

Local format controls can run without credentials:

```sh
crexx -nokeep scripts/check-apple-secrets.crexx --args test
```

The full signing check is restricted to disposable GitHub runners. Passing it
qualifies these secrets and synthetic signing operations; it does not qualify
a product build, installer execution or notarization submission.
