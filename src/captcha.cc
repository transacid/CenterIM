#include "icqconf.h"
#include "icqface.h"
#include "captcha.h"

captcha::captcha() {
}

captcha::~captcha() {
        questions_t::iterator i;

	for (i = questions.begin() ; i != questions.end() ; i++ ) {
		delete *i;
	}
}

bool captcha::docaptcha(imcontact c)
{
        if (captchas.find(c) == captchas.end()) {
		return false;
        } else {
		captchas_t::iterator i;
		captchaasked_t *ca;
		set<string> *a;
		i = captchas.find(c);
		ca = &(i->second);
		a = ca->answers;
		return true;
	}
}

void captcha::donecaptcha(imcontact c)
{
	captchas.erase(c);
}

bool captcha::checkcaptcha(imcontact c, string answer)
{
        captchas_t::iterator i;
	captchaasked_t *ca;

        i = captchas.find(c);

        if (i != captchas.end()) {
		ca = &(i->second);
		if (ca->expiry < time(NULL)) {
			captchas.erase(c);
			return false;
		} else {
			return (ca->answers->find(lo(answer)) != ca->answers->end());
		}
	}
	return false;
}

string captcha::getcaptchaquestion(imcontact c) {
	question_t *t;
	captchaasked_t ca;

	t = questions[(int)(questions.size() * (rand() / (RAND_MAX + 1.0)))];
	ca.answers = &t->answers;
	ca.expiry = conf->getcaptchatimeout() * 60 + time(NULL);
	captchas[c] = ca;

	return t->question;
}

void captcha::addquestion(string data) {
	question_t *q;
	string::size_type pos;
	

	pos = data.find("//");
	if (pos == string::npos) {
		return;
	} else {
		q = new question_t;
		q->question = data.substr(0, pos);
		data.erase(0, pos + 2);
	
		while (data.length()) {
			pos = data.find("::");
			if (pos == string::npos) {
				q->answers.insert(lo(data));
				data.erase();
			} else if (pos == 0) {
				data.erase();
			} else {
				q->answers.insert(lo(data.substr(0, pos)));
				data.erase(0, pos + 2);
			}
		}
	
		if (q->answers.empty()) {
			delete q;
		} else {
			questions.push_back(q);
		}
	}
}

bool captcha::empty()
{
	return questions.empty();
}

unsigned int captcha::size()
{
	return questions.size();
}
