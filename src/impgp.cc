#include "impgp.h"

#ifdef HAVE_GPGME

#include "icqconf.h"
#include "abstracthook.h"

impgp pgp;

impgp::impgp() {
    if(gpgme_new(&ctx))
	ctx = 0;
}

impgp::~impgp() {
    if(ctx)
	gpgme_release(ctx);
}

string impgp::getkeyinfo(const string &fp, bool secret) {
    gpgme_key_t key;
    string r;

    if(ctx && !gpgme_get_key(ctx, fp.c_str(), &key, secret ? 1 : 0)) {
	r = key->subkeys->keyid+8;
	r += ": ";
	r += key->uids->name;
	r += " (";
	r += key->uids->comment;
	r += ") <";
	r += key->uids->email;
	r += ">";
	gpgme_key_release(key);
    }

    return r;
}

vector<string> impgp::getkeys(bool secretonly) {
    gpgme_key_t key;
    vector<string> r;

    if(ctx && !gpgme_op_keylist_start(ctx, 0, secretonly ? 1 : 0)) {
	while(!gpgme_op_keylist_next(ctx, &key)) {
	    r.push_back(key->subkeys->keyid);
	    gpgme_key_release(key);
	}
    }

    return r;
}

string impgp::sign(const string &text, const string &keyid) {
    gpgme_data_t in, out;
    gpgme_key_t key;
    string r;
    size_t n;

    if(ctx) {
	gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP);
	gpgme_set_textmode(ctx, 0);
	gpgme_set_armor(ctx, 1);

	if(!gpgme_get_key(ctx, keyid.c_str(), &key, 1)) {
	    gpgme_signers_clear(ctx);
	    gpgme_signers_add(ctx, key);

	    if(!gpgme_data_new_from_mem(&in, text.c_str(), text.size(), 0)) {
		if(!gpgme_data_new(&out)) {
		    if(!gpgme_op_sign(ctx, in, out, GPGME_SIG_MODE_DETACH)) {
			auto_ptr<char> p(gpgme_data_release_and_get_mem(out, &n));
			r = p.get();
			strip(r);
		    } else {
			gpgme_data_release(out);
		    }
		}
		gpgme_data_release(in);
	    }
	    gpgme_key_release(key);
	}
    }

    return r;
}

string impgp::verify(string sign, const string &orig) {
    string r;
    gpgme_data_t dsign, dorig;
    gpgme_key_t key;
    gpgme_verify_result_t vr;

    if(ctx) {
	sign = "-----BEGIN PGP SIGNATURE-----\n\n" + sign + "\n-----END PGP SIGNATURE-----\n";

	if(!gpgme_data_new_from_mem(&dsign, sign.c_str(), sign.size(), 0)) {
	    if(!gpgme_data_new_from_mem(&dorig, orig.c_str(), orig.size(), 0)) {
		if(!gpgme_op_verify(ctx, dsign, dorig, 0)) {
		    if(vr = gpgme_op_verify_result(ctx)) {
			if(vr->signatures)
			if(!gpgme_get_key(ctx, vr->signatures->fpr, &key, 0)) {
			    r = key->subkeys->keyid;
			    gpgme_key_release(key);
			}
		    }
		}
		gpgme_data_release(dorig);
	    }
	    gpgme_data_release(dsign);
	}
    }

    return r;
}

gpgme_error_t impgp::passphrase_cb(void *hook, const char *uidhint,
const char *info, int prevbad, int fd) {
    cout << info << endl;
    return 0;
}

string impgp::decrypt(string text) {
    string r;
    gpgme_data_t in, out;
    gpgme_key_t key;
    gpgme_decrypt_result_t dr;
    size_t n;

    if(ctx) {
	text = "-----BEGIN PGP MESSAGE-----\n\n" + text + "\n-----END PGP MESSAGE-----\n";
	gpgme_set_passphrase_cb(ctx, &passphrase_cb, 0);

	if(!gpgme_data_new_from_mem(&in, text.c_str(), text.size(), 0)) {
	    if(!gpgme_data_new(&out)) {
		if(!gpgme_op_decrypt(ctx, in, out)) {
		    if(dr = gpgme_op_decrypt_result(ctx)) {
		    }

		    auto_ptr<char> buf(gpgme_data_release_and_get_mem(out, &n));
		    auto_ptr<char> p(new char[n+1]);
		    strncpy(p.get(), buf.get(), n);
		    p.get()[n] = 0;
		    r = p.get();
		} else {
		    gpgme_data_release(out);
		}
	    }
	    gpgme_data_release(in);
	}
    }

    return r;
}

string impgp::encrypt(const string &text, const string &keyid) {
    string r;
    gpgme_key_t key;
    gpgme_data_t in, out;
    size_t n;

    if(ctx) {
	gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP);
	gpgme_set_textmode(ctx, 0);
	gpgme_set_armor(ctx, 1);

	if(!gpgme_get_key(ctx, keyid.c_str(), &key, 1)) {
	    gpgme_key_t keys[] = { key, 0 };

	    if(!gpgme_data_new_from_mem(&in, text.c_str(), text.size(), 0)) {
		if(!gpgme_data_new(&out)) {
		    if(!gpgme_op_encrypt(ctx, keys, gpgme_encrypt_flags_t(0), in, out)) {
			auto_ptr<char> p(gpgme_data_release_and_get_mem(out, &n));
			r = p.get();
			strip(r);
		    } else {
			gpgme_data_release(out);
		    }
		}
		gpgme_data_release(in);
	    }
	    gpgme_key_release(key);
	}
    }

    return r;
}

bool impgp::enabled(protocolname pname) const {
    return gethook(pname).getCapabs().count(hookcapab::pgp)
	&& !conf.getourid(pname).additional["pgpkey"].empty();
}

bool impgp::enabled(const imcontact &ic) const {
    bool r = false;
    icqcontact *c;

    if(c = clist.get(ic)) {
	r = enabled(ic.pname) && c->getusepgpkey() && !c->getpgpkey().empty();
    }

    return r;
}

void impgp::strip(string &r) {
    int n;

    if((n = r.find("\n\n")) != -1)
	r.erase(0, n+2);

    if((n = r.rfind("\n")) != -1)
	r.erase(n);

    if((n = r.rfind("\n")) != -1)
	r.erase(n);
}

#endif
