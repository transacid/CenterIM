#ifndef __CAPTCHA_H__
#define __CAPTCHA_H__

#include <time.h>

class captcha {
	private:
		typedef struct {
			string question;
			set<string> answers;
		} question_t;

		typedef struct {
			time_t expiry;
			set<string> *answers;
		} captchaasked_t;

		typedef std::vector<question_t*> questions_t;
		typedef std::map<imcontact, captchaasked_t> captchas_t;

		captchas_t captchas;
		questions_t questions;
	public:
		captcha();
		~captcha();

		/* check if we are already testing this contact */
		bool docaptcha(imcontact c);
		void donecaptcha(imcontact c);
		/* check an answer */
		bool checkcaptcha(imcontact c, string answer);
		/* get a (new) question for a contact and remeber it */
		string getcaptchaquestion(imcontact c);

		bool empty();
		unsigned int size();
		void addquestion(string data);
};


#endif
