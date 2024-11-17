# Generate Secure and Recoverable Passwords Using Trezor

macer generates deterministic passwords from the Bip-39 wordlist of length
12 (128-bits), 18 (192 bits) or 24 (256-bits) derived from the Trezor seed and
a user selected `user@host`. The `user@host` is displayed on the Trezor with
the phrase `MACER decrypt` for verification before the password can be
generated. Another Trezor with the same seed and URI will generate the same
password, making backup of high-entropy passwords easier.

> The bip39-12 output is likely sufficient for many use cases. This is
> equivalent to [10-diceware-words](https://theworld.com/~reinhold/diceware.html).

> The last word of each bip39 output mode contains a small checksum. This is
> only useful when using bip39 compatible software. Users can manually truncate
> words if the pre-selected word counts doesn't meet user needs.

Detractors will point out that macer is limited to 125.8 bit strength  due to
x25519 usage. HOWEVER, an attacker has to first obtain a 252-bit x25519
public-key to crack the 252-bit secret key, AND the public-key cracking method
_cannot be parallelized_. Brute-force searching 12 words/128-bits can be
parallelized, making 18/24 macer words useful in some contexts (thus the option
for 18/24 words). The situation is analogous to the common usage of
x25519+AES-256 instead of x25519+AES-128. See [security section](#security) for
further analysis.

> Brute-force of x25519 public-keys can be sped up by multi-core CPUs, but the
> process is 2^252. Whereas the 2^125.8 cracking method for x25519 (rho) can
> only be run in serial. Again, see [security section](#security) for details.

> macer can also output the raw 32-byte binary secret, but users must be
> careful about the newline character stopping `stdin` reading in some
> applications. The primary use case is to run `macer -f binary | base58`,
> which generates safe to copy 44-character passwords.
> `macer -f binary | head -c 16 | base58` will generate 22-character passwords.
> The `base58` utility is recommended because `0`, `O`, `I` and `1` are **not**
> used, making paper backups more reliable.

## Motivation

### Recovery from Major Possession Loss

Recovering data - after a major flood or fire - is problematic with typical
password based setups. In a major event, paper/digital backup of passwords
could be permanently/destroyed lost (was the safe properly rated for the
fire?), in addition to all other physical items. You can make easy to remember
passwords for this situation, but then typically the password has low entropy
making it unsuitable for encryption purposes. In contrast, Bip-39 (Bitcoin)
seeds have high entropy, and are commonly stored in multiple physical locations
to prevent this worst case outcome. Remembering 1 or 2 URIs, and getting access
to the "backup" Bip-39 seed can bootstrap recovery for the remainder of your
personal cloud data!

As an example, use macer to generate `me@proton.me` for Proton Drive, then
`me@keepass` for a local keepass instance with the filename
`me_at_keepass.kdbx`, and manually store the file on Proton Drive. Recovery
only requires proper seed backup + remembering the URI `me@proton.me` (the
other URI is stored in the filename on Proton Drive!). Further recovery is
performed based on information in the keepass file.

One recommendation is to use keepass/lastpass to generate "basic"
(website/banking) secrets, but use macer to generate secrets for crypto
purposes WITHOUT storing the secrets in the cloud. This way a
crypto/implementation break on cloud encrypted data doesn't necessarily leak
_all other crypto passwords_. 

> Lastpass, Bitwarden or Proton Pass could be used instead of the Keepass+drive
> combo, with the downside that local backups are always exported in
> unencrypted formats. Keepass+drive skips this problem, with the downside of
> the process being more manual. Use whichever you find easier in recovery
> scenarios.

> [seedpass](https://seedpass.me) can be used with Coldcard+BIP85 OR macer
> with `me@seedpass.me` as the URI. The major downside of seedpass is that the
> parent seed used to generate every child password is encrypted locally with a
> password. If the machine is compromised, every password will leak if an
> insufficient password is used. In contrast, macer never stores any secret
> material on the host machine, everything is re-generated on-the-fly from
> Trezor. That said, a user can _probably_ get away with just a 90-bit password
> for the seedpass password. Use [diceware](https://theworld.com/~reinhold/diceware.html)
> for this password, there are only downsides to using macer here. One last
> drawback of seedpass - you need to store an associated encrypted metafile
> somewhere, or recovering passwords is going to be problematic. The default
> uses nostr for backup+sync, which is slick because this encypted metadata
> file contains no passwords, so posting publicly is of low concern.

### LUKS (Linux encrypted disk)

macer can be used in the Linux initrd script to output a password from the
Trezor and pipe it directly into `cryptsetup`. The initrd script tracks the
correct `user@host` information, which is displayed on the Trezor, so there is
less risk of exposing the password for another device or service. Seedpass is
less useful in this context because it really isn't intended to be run in an
initrd script. The current recommendation for macer+LUKS is: (1) use it if you
hate typing long passwords to LUKS OR (2) are confident that you can physically
backup a 24-word seed but are less confident in backing up many 20-word
[diceware](https://theworld.com/~reinhold/diceware.html) passwords. LUKS users
can also use both macer and diceware simultaneously if the
[limitations](#limitations) of macer are a concern. One thing to note is that
the primary motivation - total possession loss - would typically mean all your
LUKS devices are gone anyway, so for many people macer is only useful when (1)
applies.


## Implementation Details

macer uses the SLIP-17 x25519 ECDH feature of Trezor, and communicates with the device
using libusb-1.0.0 and custom C++11 read/write code. macer can be statically
linked for use in initrd or other limited environments. macer does not depend
on Python, Protobuf, or Trezor source code for building.

SLIP-17 ECDH generates a shared secret; a public-key+URI combination is
required for the operation. macer first requests a public-key for the URI
`macer_peerkey://user@host` using the same URI->Bip-32-path algorithm described
in SLIP-17. The public-key returned by the request is then fed into the actual
ECDH request with the URI `macer://user@host`. The shared-secret is then hashed
by SHA-256 and the first 16, 24 or 32 bytes of the hash (depends on user
configuration) is converted to 12, 18 or 24 words using the algorithm defined
by Bip-39.

### Security

macer arguably has 252 bits of entropy, despite x25519 commonly described as
having 125.8 bits of security. The 125.8 bit number refers to the
number of operations needed to "crack" known public key(s). In most scenarios,
an attacker will not have a public-key to "crack", so no information is
"leaked" about the private key. This leaves ~2^252 possible combinations for
the input into SHA-256.

The major CAVEAT is that an attacker who compromises a host machine of the user
can request a x25519 public key from the Trezor without confirmation on the
device. This will (assuming the attacker can determine URIs in use) lower the
security of associated passwords to ~125.8 bits. HOWEVER, cracking an x25519
public key with rho cannot be parallelized where symmetric key brute forcing
can be parallelized. If just 12 Bip-39 words are used for AES encryption, then
the time required to brute force is `2^128/p` where `p` is the number of
parallel "circuits" (CPU cores) used for searching. In contrast, the time for
the rho method for x25519 public-key cracking will take `2^126/1` because the
algorithm is serial. This
[stackoverflow post](https://crypto.stackexchange.com/questions/59762/after-ecdh-with-curve25519-is-it-pointless-to-use-anything-stronger-than-aes-12)
is related as x25519 is best paired with AES-256.

If your password gets fed into `argon2` (or similar), this will increase the
cost of a search circuit, but doesn't increase the search space. Whether
12-words is sufficient in this context is up to the reader. 18/24 words does
have some merit - an attacker is better of with a brute-force search of (1)
12-words if they can afford 8+ search circuits, (2) 18-words if they can afford
2^67+ search circuits, and (3) 24-words if they can afford 2^127+ search
circuits. The analysis gets more complicated when multi-key cracking gets
introduced, but brute-force searching does quite well in this context.

> As a point of reference, a supercomputer in China can reduce brute-force
> search _time_ by 2^23. Custom built machines can likely do better.

Lastly, users of 12-seeds into the Trezor probably won't find 24 macer words
particularly useful. They should likely stick to 12 macer words.


### Hashing URI is only 128-bits.

Internally on a Trezor, the URI is given to SHA256 and only the first 128-bits
are used to generate the "path" that determines the x25519 secret key. This
does not reduce the entropy of the passwords to 128-bits in the same way that
an attacker knowing that the Bip-32 path for a users funds is `m/44'/0'/0/0`
does not reduce the entropy of the secret key to zero. It does however make the
probability of two URIs "colliding" reasonably high due to the
"birthday attack". The biggest problem would be the attacker finding a
collision, then convincing you to use that collision in their service. This is
going to be somewhat difficult because they wouldn't necessarily control the
user or host portion after finding a collision. If SHA256 breaks in the same
way md5 did, then perhaps this needs to be re-visted, but this is a non-issue
(although I wish Trezor used all 256-bits for this use case).


## Limitations + Mitigations

### Firmware Changes

macer uses the ECDH function of Trezor to generate secure passwords. This
hardware mode is unlikely to change as it is well defined in SLIP-17, and a
major change could leave people unable to decrypt documents. Cautious users
may want to (1) use seedpass and/or (2) use paper/digital backups. Using paper
backups is likely the better options for most people (unfortunately). In both
cases, the basic technique is the same, generate and store the output of
`macer -f bip39-24 -t example.com -u user > test_pass`. Then after firmware
update, run `macer -f bip39-24 -t example.com -u user | diff test_pass -`. If
nothing is output by `diff`, then macer passwords will not change. Otherwise,
you must use your seedpass or paper/digital backups to migrate all passwords
to their new versions.

> Digital backups should arguably be local-only to reduce leaks from crypto
> failures. This means storing info about macer passwords twice - once storing
> the URI without password in the cloud, and once with password locally using
> keepass. Another concern (with digital backups) is that the host machine
> learns about passwords generated for other machines. This is why paper
> backups are the better option.

> See [motivations](#motivations) section on how to use seedpass with macer.
> One additional note on seedpass: you may want a paper backup of the seedpass
> input as an extra pre-caution. This method would reduce the number of paper
> backups to just 2 - the Trezor seed and seedpass seed.

> Both backup techniques don't "ruin" the primary motivation of this project -
> losing paper backups is only problematic when Trezor simultaneously changes
> firmware or goes out of business. And even then, software emulation can
> mitigate either of these issues. In other words, use macer to generate secure
> recoverable secrets, but treat them like diceware generated passwords.


### Software Emulation with Coin Usage

A purely software implementation will likely be made in the future, so users
can recover passwords without a Trezor device. This will also reduce the
firmware change issue. If coins are stored on the same device, then bad things
could happen when using the software emulation.

The best mitigation strategy is to use the technique in the firmware changes
section - be proactive about detection and rotating macer passwords. If you
failed to do this properly, you can "sweep" your coins before using the
software emulation.

### U2F/2FA Usage

Using macer and U2F (aka 2FA) with the same Trezor is _possibly_ problematic -
you have no "2nd factor" because one device is generating both secrets.
However, if an attacker gets access to your laptop, the attacker gets the macer
generated passwords, but cannot "replay" the U2F generated signature. The big
problem with U2F is that Trezor cannot display the URI (upstream protocol
limitation), which is likely why passkeys are being promoted as a replacement.

Possible mitigation strategies are generating unique/independent passwords
for services or using a second Trezor device. Both have their limitations with
backup; using macer+cloud-backup is still arguably the same 2FA issue.

I think for most users, macer+U2F is acceptable.

### Password "Destruction"

Once a password is generated using macer, it can always be re-generated until
the Bip-39 seed is destroyed. In contrast, a
[diceware](https://theworld.com/~reinhold/diceware.html) generated password
and stored only on paper can be destroyed via physical shredding/burning.
 
Don't use macer (or seedpass) if you are concerned about long-term access to
passwords - there is no other mitigation unfortunately.




## Building

### Requirements
 * Autotools (when from Github/source)
 * Bash
 * make
 * libusb-1.0.0
 * A C++11 compatible compiler

### From Github
```bash
git clone ssh://git@github.com/cifro-codes/macer.git
cd macer
autoreconf -i && ./configure && make
```

### From Source Tarball
```bash
tar xvf macer-0.2.tar.bz
cd macer
./configure && make
```

### Static Builds
Change the `./configure` steps above with `./configure LDFLAGS="-static"`. This
will fail on many systems because libusb is not provided statically (Gentoo is
a common exception). Help will not be provided for this setup, because
generating a custom initrd script means you can probably solve this problem.

## Usage

Run `macer --help` to get options and descriptions. Self explanatory
if this README.md was read from beginning.

```bash
	--help, -h			List help
	--existing, -e			Prompt for existing LUKS password for adding new key
	--format, -f	[format]	Output format/strength -> legacy | binary | bip39-12 | bip39-18 | bip39-24
	--host, -t	[hostname]	Identity hostname for password
	--user, -u	[user]		Identity username for password
	--message, -m	[message]	Message to display on device (legacy format only)
	--password, -p			Prompt for local only password to append to stdout (more entropy)
```
