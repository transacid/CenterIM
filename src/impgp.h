#ifndef __IMPGP_H__
#define __IMPGP_H__

#include "icqcommon.h"

#ifdef HAVE_GPGME

#include "imcontact.h"

#include <gpgme.h>

class impgp {
    private:
	gpgme_ctx_t ctx;

	static protocolname opname;
	static string passphrase[protocolname_size];

	void strip(string &r);

	static gpgme_error_t passphrase_cb(void *hook, const char *uidhint,
	    const char *info, int prevbad, int fd);

    public:
	impgp();
	~impgp();

	vector<string> getkeys(bool secretonly = false);
	string getkeyinfo(const string &fp, bool secret);
	bool havekey(const string &keyid) const;

	string sign(const string &text, const string &keyid, protocolname pname);
	string verify(string sign, const string &orig);

	string decrypt(string text, protocolname pname);
	string encrypt(const string &text, const string &keyid, protocolname pname);

	bool enabled(protocolname p) const;
	bool enabled(const imcontact &ic) const;

	void clearphrase(protocolname p);
};

extern impgp pgp;

#endif

#endif
