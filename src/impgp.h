#ifndef __IMPGP_H__
#define __IMPGP_H__

#include "icqcommon.h"

#ifdef HAVE_GPGME

#include "imcontact.h"

#include <gpgme.h>

class impgp {
    private:
	gpgme_ctx_t ctx;

	void strip(string &r);

	static gpgme_error_t passphrase_cb(void *hook, const char *uidhint,
	    const char *info, int prevbad, int fd);

    public:
	impgp();
	~impgp();

	vector<string> getkeys(bool secretonly = false);
	string getkeyinfo(const string &fp, bool secret);

	string sign(const string &text, const string &keyid);
	string verify(string sign, const string &orig);

	string decrypt(string text);
	string encrypt(const string &text, const string &keyid);

	bool enabled(protocolname p) const;
	bool enabled(const imcontact &ic) const;
};

extern impgp pgp;

#endif

#endif
