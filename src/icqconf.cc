/*
*
* centericq configuration handling routines
* $Id: icqconf.cc,v 1.143 2005/09/02 15:20:59 konst Exp $
*
* Copyright (C) 2001-2004 by Konstantin Klyagin <konst@konst.org.ua>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at
* your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
* USA
*
*/

#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fstream>

#ifdef __sun__
#include <sys/statvfs.h>
#endif

#include "icqconf.h"
#include "icqface.h"
#include "icqcontacts.h"
#include "abstracthook.h"
#include "imexternal.h"
#include "eventmanager.h"
#include "imlogger.h"
#include "connwrap.h"

icqconf::icqconf() {
    rs = rscard;
    rc = rcdark;
    cm = cmproto;
    fgroupmode = nogroups;

    autoaway = autona = 0;

    hideoffline = antispam = makelog = askaway = logtimestamps =
	logonline = emacs = proxyssl = proxyconnect = notitles =
	debug = false;

    startoffline = false;

    savepwd = mailcheck = fenoughdiskspace = true;

    for(protocolname pname = icq; pname != protocolname_size; pname++) {
	chatmode[pname] = true;
	cpconvert[pname] = entersends[pname] = nonimonline[pname] = false;
    }

    basedir = (string) getenv("HOME") + "/.centericq/";
}

icqconf::~icqconf() {
}

icqconf::imaccount icqconf::getourid(protocolname pname) const {
    imaccount im;
    vector<imaccount>::const_iterator i;

    i = find(accounts.begin(), accounts.end(), pname);
    im = i == accounts.end() ? imaccount(pname) : *i;

    return im;
}

icqconf::imserver icqconf::defservers[protocolname_size] = {
    { "login.icq.com", 5190, 0 },
    { "scs.msg.yahoo.com", 5050, 0 },
    { "messenger.hotmail.com", 1863, 0 },
    { "toc.oscar.aol.com", 9898, 0 },
    { "irc.oftc.net", 6667, 0 },
    { "jabber.org", 5222, 5223 },
    { "", 0, 0 },
    { "livejournal.com", 80, 0 },
    { "appmsg.gadu-gadu.pl", 80 }
};

void icqconf::setourid(const imaccount &im) {
    vector<imaccount>::iterator i;

    i = find(accounts.begin(), accounts.end(), im.pname);

    if(i != accounts.end()) {
	if(im.empty()) {
	    accounts.erase(i);
	    i = accounts.end();
	} else {
	    *i = im;
	}
    } else if(!im.empty()) {
	accounts.push_back(im);
	i = accounts.end()-1;
    }

    if(i != accounts.end()) {
	if(i->server.empty()) {
	    i->server = defservers[i->pname].server;
	    i->port = defservers[i->pname].port;
	}

	if(!i->port)
	    i->port = defservers[i->pname].port;

	switch(i->pname) {
	    case jabber:
		if(i->additional.find("prio") == i->additional.end())
		    i->additional["prio"] = "4";
		break;

	    case livejournal:
		if(i->additional.find("importfriends") == i->additional.end())
		    i->additional["importfriends"] = "1";
		break;

	    case icq:
		if(i->password.size() > 8)
		    i->password = i->password.substr(0, 8);
		break;
	}
    }
}

string icqconf::getawaymsg(protocolname pname) const {
    string r, buf, fname;
    ifstream f;

    if(!gethook(pname).getCapabs().count(hookcapab::setaway))
	return "";

    fname = conf.getconfigfname("awaymsg-" + getprotocolname(pname));
    f.open(fname.c_str());

    if(f.is_open()) {
	while(!f.eof()) {
	    getstring(f, buf);
	    if(!r.empty()) r += "\n";
	    r += buf;
	}

	f.close();
    }

    if(r.empty()) {
	char buf[512];

	sprintf(buf, _("I do really enjoy the default %s away message of %s %s."),
	    getprotocolname(pname).c_str(), PACKAGE, VERSION);

	return buf;
    }

    return r;
}

void icqconf::setawaymsg(protocolname pname, const string &amsg) {
    string fname;
    ofstream f;

    if(!gethook(pname).getCapabs().count(hookcapab::setaway))
	return;

    fname = conf.getconfigfname("awaymsg-" + getprotocolname(pname));
    f.open(fname.c_str());

    if(f.is_open()) {
	f << amsg;
	f.close();
    }
}

void icqconf::checkdir() {
    string dname = getdirname();
    dname.erase(dname.size()-1);

    if(access(dname.c_str(), F_OK))
	mkdir(dname.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
}

void icqconf::load() {
    loadmainconfig();
    loadkeys();
    loadcolors();
    loadactions();
    external.load();
}

vector<icqconf::keybinding> icqconf::keys;

void icqconf::loadkeys() {
    int x;
    string fname = getconfigfname("keybindings"), buf, param;
    struct keybinding k;
    ifstream f;

    keys.clear();

    if(access(fname.c_str(), F_OK)) {
	ofstream of(fname.c_str());

	if(of.is_open()) {
	    of << "# This file contains keybinding configuration for centericq" << endl;
	    of << "# Every line should look like: bind <section> <key> <command>" << endl << endl;
	    of << "bind contact\t?\tinfo" << endl;
	    of << "bind contact\tq\tquit" << endl;
	    of << "bind contact\t<F3>\tchange_status" << endl;
	    of << "bind contact\ts\tchange_status" << endl;
	    of << "bind contact\th\thistory" << endl;
	    of << "bind contact\tf\tfetch_away_message" << endl;
	    of << "bind contact\t<F2>\tuser_menu" << endl;
	    of << "bind contact\tm\tuser_menu" << endl;
	    of << "bind contact\t<F4>\tgeneral_menu" << endl;
	    of << "bind contact\tg\tgeneral_menu" << endl;
	    of << "bind contact\t<F5>\thide_offline" << endl;
	    of << "bind contact\t<F6>\tcontact_external_action" << endl;
	    of << "bind contact\ta\tadd" << endl;
	    of << "bind contact\t<delete> remove" << endl;
	    of << "bind contact\t<enter> compose" << endl;
	    of << "bind contact\tc\tsend_contact" << endl;
	    of << "bind contact\tc\trss_check" << endl;
	    of << "bind contact\tu\tsend_url" << endl;
	    of << "bind contact\tr\trename" << endl;
	    of << "bind contact\tv\tversion" << endl;
	    of << "bind contact\te\tedit" << endl;
	    of << "bind contact\ti\tignore" << endl;
	    of << "bind contact\t\\as\tquickfind" << endl;
	    of << "bind contact\t/\tquickfind" << endl << endl;
	    of << "bind contact\t\\cn\tnext_chat" << endl;
	    of << "bind contact\t\\cb\tprev_chat" << endl;
	    of << "bind history\t/\tsearch" << endl;
	    of << "bind history\ts\tsearch" << endl;
	    of << "bind history\tn\tsearch_again" << endl;
	    of << "bind history\t<F2>\tshow_urls" << endl;
	    of << "bind history\t<F9>\tfullscreen" << endl << endl;
	    of << "bind editor\t\\cx\tsend_message" << endl;
	    of << "bind editor\t<esc>\tquit" << endl;
	    of << "bind editor\t\\cp\tmultiple_recipients" << endl;
	    of << "bind editor\t\\co\thistory" << endl;
	    of << "bind editor\t\\cn\tnext_chat" << endl;
	    of << "bind editor\t\\cb\tprev_chat" << endl;
	    of << "bind editor\t\\a?\tinfo" << endl;
	    of << "bind editor\t<F2>\tshow_urls" << endl;
	    of << "bind editor\t<F9>\tfullscreen" << endl << endl;
	    of << "bind info\t<F2>\tshow_urls" << endl;
	    of << "bind info\t<F6>\tuser_external_action" << endl;

	    of.close();
	}
    }

    f.open(fname.c_str());

    if(f.is_open()) {
	while(!f.eof()) {
	    getstring(f, buf);

	    if(buf.empty() || buf[0] == '#') continue;

	    k.ctrl = false;
	    k.alt = false;

	    param = getword(buf);       /* first word is always bind */

	    param = getword(buf);       /* read section */
	    if(param == "contact")
		k.section = section_contact; else
	    if(param == "history")
		k.section = section_history; else
	    if (param == "editor")
		k.section = section_editor; else
	    if (param == "info")
		k.section = section_info;

	    param = getword(buf);       /* key */
	    if(param == "\\c<space>" || param == "\\C<space>" || param == "^<space>") {
		k.ctrl = true;
		k.key = CTRL(' ');
	    } else if(param == "\\a<enter>" || param == "\\A<enter>" || param == "@<enter>") {
		k.alt = true;
		k.key = ALT('\r');
	    } else if(param == "\\a<space>" || param == "\\A<space>" || param == "@<space>") {
		k.ctrl = true;
		k.key = ALT(' ');
	    } else if(param[0] == '\\') {
		if(param[1] == 'c' || param[1] == 'C') {
		    k.ctrl = true;
		    k.key = CTRL(param[2]);
		} else if(param[1] == 'a' || param[1] == 'A') {
		    k.alt = true;
		    k.key = ALT(param[2]);
		} else if(param[1] == '\\') {
		    k.key = '\\';
		} else if(param[1] == '@') {
		    k.key = '@';
		} else if(param[1] == '^') {
		    k.key = '^';
		}
	    } else if(param[0] == '^') {
		k.ctrl = true;
		k.key = CTRL(param[1]);
	    } else if(param[0] == '@') {
		k.alt = true;
		k.key = ALT(param[1]);
	    } else if(param == "<delete>")
		k.key = KEY_DC;
	    else if(param == "<enter>")
		k.key = '\r';
	    else if(param == "<esc>")
		k.key = 27;
	    else if(param == "<space>")
		k.key = ' ';
	    else if(param == "<backspace>")
		k.key = '\b';
	    else if(param == "<pageup>")
		k.key = 339;
	    else if(param == "<pagedown>")
		k.key = 338;
	    else if(param == "<tab>")
		k.key = 9;
	    else if(param == "<insert>")
		k.key = 331;
	    else if(param[0] == '<' && param[1] == 'F') {
		k.alt = true;
		k.ctrl = true;
		k.key = KEY_F(param[2]-'0');
	    } else
		k.key = param[0];

	    param = getword(buf);       /* action */
	    if(param == "info") k.action = key_info; else
	    if(param == "remove") k.action = key_remove; else
	    if(param == "quit") k.action = key_quit; else
	    if(param == "change_status") k.action = key_change_status; else
	    if(param == "history") k.action = key_history; else
	    if(param == "next_chat") k.action = key_next_chat; else
	    if(param == "prev_chat") k.action = key_prev_chat; else
	    if(param == "fetch_away_message") k.action = key_fetch_away_message; else
	    if(param == "user_menu") k.action = key_user_menu; else
	    if(param == "general_menu") k.action = key_general_menu; else
	    if(param == "hide_offline") k.action = key_hide_offline; else
	    if(param == "contact_external_action") k.action = key_contact_external_action; else
	    if(param == "add") k.action = key_add; else
	    if(param == "remove") k.action = key_remove; else
	    if(param == "send_message") k.action = key_send_message; else
	    if(param == "send_contact") k.action = key_send_contact; else
	    if(param == "send_url") k.action = key_send_url; else
	    if(param == "rename") k.action = key_rename; else
	    if(param == "version") k.action = key_version; else
	    if(param == "fullscreen") k.action = key_fullscreen; else
	    if(param == "edit") k.action = key_edit; else
	    if(param == "ignore") k.action = key_ignore; else
	    if(param == "quickfind") k.action = key_quickfind; else
	    if(param == "search") k.action = key_search; else
	    if(param == "compose") k.action = key_compose; else
	    if(param == "search_again") k.action = key_search_again; else
	    if(param == "show_urls") k.action = key_show_urls; else
	    if(param == "rss_check") k.action = key_rss_check; else
	    if(param == "multiple_recipients") k.action = key_multiple_recipients; else
	    if(param == "user_external_action") k.action = key_user_external_action;
		else continue;

	    keys.push_back(k);
	}

	f.close();
    }
/*
    if(face.action2key(key_info, section_contact).empty()) keys.push_back(keybinding()); else
    if(face.action2key(key_quit, section_contact).empty()) 
    if(face.action2key(key_change_status, section_contact).empty()) 
    if(face.action2key(key_history, section_contact).empty()) 
    if(face.action2key(key_fetch_away_message, section_contact).empty()) 
    if(face.action2key(key_user_menu, section_contact).empty()) 
    if(face.action2key(key_general_menu, section_contact).empty()) 
    if(face.action2key(key_hide_offline, section_contact).empty()) 
    if(face.action2key(key_contact_external_action, section_contact).empty()) 
    if(face.action2key(key_add, section_contact).empty()) 
    if(face.action2key(key_remove, section_contact).empty()) 
    if(face.action2key(key_compose, section_contact).empty()) 
    if(face.action2key(key_send_contact, section_contact).empty()) 
    if(face.action2key(key_rss_check, section_contact).empty()) 
    if(face.action2key(key_send_url, section_contact).empty()) 
    if(face.action2key(key_rename, section_contact).empty()) 
    if(face.action2key(key_version, section_contact).empty()) 
    if(face.action2key(key_edit, section_contact).empty()) 
    if(face.action2key(key_ignore, section_contact).empty()) 
    if(face.action2key(key_quickfind, section_contact).empty()) 
    if(face.action2key(key_next_chat, section_contact).empty()) 
    if(face.action2key(key_prev_chat, section_contact).empty()) 
    if(face.action2key(key_search, section_history).empty()) 
    if(face.action2key(key_search_again, section_history).empty()) 
    if(face.action2key(key_show_urls, section_history).empty()) 
    if(face.action2key(key_fullscreen, section_history).empty()) 
    if(face.action2key(key_send_message, section_editor).empty()) 
    if(face.action2key(key_quit, section_editor).empty()) 
    if(face.action2key(key_multiple_recipients, section_editor).empty()) 
    if(face.action2key(key_history, section_editor).empty()) 
    if(face.action2key(key_next_chat, section_editor).empty()) 
    if(face.action2key(key_prev_chat, section_editor).empty()) 
    if(face.action2key(key_info, section_editor).empty()) 
    if(face.action2key(key_show_urls, section_editor).empty()) 
    if(face.action2key(key_fullscreen, section_editor).empty()) 
    if(face.action2key(key_show_urls, section_info).empty()) 
    if(face.action2key(key_user_external_action, section_info).empty()) {}
*/
}

void icqconf::loadmainconfig() {
    string fname = getconfigfname("config"), buf, param, rbuf;
    ifstream f(fname.c_str());
    imaccount im;
    protocolname pname;

    if(f.is_open()) {
	mailcheck = askaway = false;
	savepwd = bidi = true;
	setsmtphost("");
	setbrowser("");
	setpeertopeer(0, 65535);

	while(!f.eof()) {
	    getstring(f, buf);
	    rbuf = buf;
	    param = getword(buf);

	    if(param == "hideoffline") hideoffline = true; else
	    if(param == "emacs") emacs = true; else
	    if(param == "autoaway") autoaway = atol(buf.c_str()); else
	    if(param == "autona") autona = atol(buf.c_str()); else
	    if(param == "antispam") antispam = true; else
	    if(param == "mailcheck") mailcheck = true; else
	    if(param == "quotemsgs") quote = true; else
	    if(param == "sockshost") setsockshost(buf); else
	    if(param == "socksusername") socksuser = buf; else
	    if(param == "sockspass") sockspass = buf; else
	    if((param == "usegroups") || (param == "group1")) fgroupmode = group1; else
	    if(param == "group2") fgroupmode = group2; else
	    if(param == "protocolormode") cm = icqconf::cmproto; else
	    if(param == "statuscolormode") cm = icqconf::cmstatus; else
	    if(param == "smtp") setsmtphost(buf); else
	    if(param == "browser") setbrowser(browser); else
	    if(param == "http_proxy") sethttpproxyhost(buf); else
	    if(param == "log") makelog = true; else
	    if(param == "proxy_connect") proxyconnect = true; else
	    if(param == "proxy_ssl") proxyssl = true; else
	    if(param == "chatmode") initmultiproto(chatmode, buf, true); else
	    if(param == "entersends") initmultiproto(entersends, buf, true); else
	    if(param == "nonimonline") initmultiproto(nonimonline, buf, false); else
	    if(param == "russian" || param == "convert") initmultiproto(cpconvert, buf, false); else
	    if(param == "nobidi") setbidi(false); else
	    if(param == "askaway") askaway = true; else
	    if(param == "logtimestamps") logtimestamps = true; else
	    if(param == "logonline") logonline = true; else
	    if(param == "fromcharset") fromcharset = buf; else
	    if(param == "tocharset") tocharset = buf; else
	    if(param == "ptp") {
		ptpmin = atoi(getword(buf, "-").c_str());
		ptpmax = atoi(buf.c_str());
	    } else {
		for(pname = icq; pname != protocolname_size; pname++) {
		    buf = getprotocolname(pname);
		    if(param.substr(0, buf.size()) == buf) {
			im = getourid(pname);
			im.read(rbuf);
			setourid(im);
		    }
		}
	    }
	}

	if(fromcharset.empty() && tocharset.empty())
	for(pname = icq; pname != protocolname_size; pname++) {
	    if(getcpconvert(pname)) {
		fromcharset = "cp1251";
		tocharset = "koi8-r";
		break;
	    }
	}

	setproxy();
	f.close();
    }
}

void icqconf::save() {
    string fname = getconfigfname("config"), param;
    int away, na;

    if(enoughdiskspace()) {
	ofstream f(fname.c_str());

	if(f.is_open()) {
	    if(!getsockshost().empty()) {
		string user, pass;
		getsocksuser(user, pass);
		f << "sockshost\t" << getsockshost() + ":" + i2str(getsocksport()) << endl;
		f << "socksusername\t" << user << endl;
		f << "sockspass\t" << pass << endl;
	    }

	    getauto(away, na);

	    if(away) f << "autoaway\t" << i2str(away) << endl;
	    if(na) f << "autona\t" << i2str(na) << endl;
	    if(hideoffline) f << "hideoffline" << endl;
	    if(emacs) f << "emacs" << endl;
	    if(getquote()) f << "quotemsgs" << endl;
	    if(getantispam()) f << "antispam" << endl;
	    if(getmailcheck()) f << "mailcheck" << endl;
	    if(getaskaway()) f << "askaway" << endl;

	    param = "";
	    for(protocolname pname = icq; pname != protocolname_size; pname++)
		if(getchatmode(pname)) param += (string) " " + conf.getprotocolname(pname);
	    if(!param.empty())
		f << "chatmode" << param << endl;

	    param = "";
	    for(protocolname pname = icq; pname != protocolname_size; pname++)
		if(getentersends(pname)) param += (string) " " + conf.getprotocolname(pname);
	    if(!param.empty())
		f << "entersends" << param << endl;

	    param = "";
	    for(protocolname pname = icq; pname != protocolname_size; pname++)
		if(getnonimonline(pname)) param += (string) " " + conf.getprotocolname(pname);
	    if(!param.empty())
		f << "nonimonline" << param << endl;

	    param = "";
	    for(protocolname pname = icq; pname != protocolname_size; pname++)
		if(getcpconvert(pname)) param += (string) " " + conf.getprotocolname(pname);
	    if(!param.empty())
		f << "convert" << param << endl;

	    f << "fromcharset\t" << fromcharset << endl;
	    f << "tocharset\t" << tocharset << endl;

	    if(!getbidi()) f << "nobidi" << endl;
	    if(logtimestamps) f << "logtimestamps" << endl;
	    if(logonline) f << "logonline" << endl;

	    f << "smtp\t" << getsmtphost() << ":" << getsmtpport() << endl;
	    f << "browser\t" << getbrowser() << endl;

	    if(!gethttpproxyhost().empty())
		if (!gethttpproxyuser().empty())
		    f << "http_proxy\t" << gethttpproxyuser() << ":" << gethttpproxypasswd() << "@" << gethttpproxyhost() << ":" << gethttpproxyport() << endl;
		else
		    f << "http_proxy\t" << gethttpproxyhost() << ":" << gethttpproxyport() << endl;

	    f << "ptp\t" << ptpmin << "-" << ptpmax << endl;

	    switch(getgroupmode()) {
		case group1: f << "group1" << endl; break;
		case group2: f << "group2" << endl; break;
	    }

	    switch(getcolormode()) {
		case cmproto:
		    f << "protocolormode" << endl;
		    break;
		case cmstatus:
		    f << "statuscolormode" << endl;
		    break;
	    }

	    if(getmakelog()) f << "log" << endl;
	    if(getproxyconnect()) f << "proxy_connect" << endl;
	    if(getproxyssl()) f << "proxy_ssl" << endl;

	    vector<imaccount>::iterator ia;
	    for(ia = accounts.begin(); ia != accounts.end(); ++ia)
		ia->write(f);

	    f.close();
	}
    }
}

void icqconf::loadcolors() {
    string tname = getconfigfname("colorscheme");

    switch(conf.getregcolor()) {
	case rcdark:
	    schemer.push(cp_status, "status black/white");
	    schemer.push(cp_dialog_text, "dialog_text   black/white");
	    schemer.push(cp_dialog_menu, "dialog_menu   black/white");
	    schemer.push(cp_dialog_selected, "dialog_selected   white/transparent   bold");
	    schemer.push(cp_dialog_highlight, "dialog_highlight red/white");
	    schemer.push(cp_dialog_frame, "dialog_frame blue/white");
	    schemer.push(cp_main_text, "main_text   cyan/transparent");
	    schemer.push(cp_main_menu, "main_menu   green/transparent");
	    schemer.push(cp_main_selected, "main_selected   black/white");
	    schemer.push(cp_main_highlight, "main_highlight yellow/transparent  bold");
	    schemer.push(cp_main_frame, "main_frame blue/transparent    bold");
	    schemer.push(cp_main_history_incoming, "main_history_incoming  yellow/transparent    bold");
	    schemer.push(cp_main_history_outgoing, "main_history_outgoing  cyan/transparent");
	    schemer.push(cp_clist_icq, "clist_icq   green/transparent");
	    schemer.push(cp_clist_yahoo, "clist_yahoo   magenta/transparent");
	    schemer.push(cp_clist_infocard, "clist_infocard white/transparent");
	    schemer.push(cp_clist_msn, "clist_msn   cyan/transparent");
	    schemer.push(cp_clist_aim, "clist_aim   yellow/transparent");
	    schemer.push(cp_clist_irc, "clist_irc    blue/transparent");
	    schemer.push(cp_clist_jabber, "clist_jabber    red/transparent");
	    schemer.push(cp_clist_rss, "clist_rss    white/transparent   bold");
	    schemer.push(cp_clist_lj, "clist_lj    cyan/transparent   bold");
	    schemer.push(cp_clist_gadu, "clist_gg    blue/transparent   bold");
	    schemer.push(cp_clist_online, "clist_online   green/transparent   bold");
	    schemer.push(cp_clist_offline, "clist_offline   red/transparent   bold");
	    schemer.push(cp_clist_away, "clist_away   yellow/transparent   bold");
	    schemer.push(cp_clist_dnd, "clist_dnd   cyan/transparent   bold");
	    schemer.push(cp_clist_na, "clist_na   blue/transparent   bold");
	    schemer.push(cp_clist_free_for_chat, "clist_free_for_chat   green/transparent   bold");
	    schemer.push(cp_clist_invisible, "clist_invisible   grey/transparent   bold");
	    schemer.push(cp_clist_not_in_list, "clist_not_in_list   white/transparent   bold");
	    break;

	case rcblue:
	    schemer.push(cp_status, "status  black/white");
	    schemer.push(cp_dialog_text, "dialog_text black/cyan");
	    schemer.push(cp_dialog_menu, "dialog_menu black/cyan");
	    schemer.push(cp_dialog_selected, "dialog_selected white/black bold");
	    schemer.push(cp_dialog_highlight, "dialog_highlight    white/cyan  bold");
	    schemer.push(cp_dialog_frame, "dialog_frame    black/cyan");
	    schemer.push(cp_main_text, "main_text   white/blue");
	    schemer.push(cp_main_menu, "main_menu   white/blue");
	    schemer.push(cp_main_selected, "main_selected   black/cyan");
	    schemer.push(cp_main_highlight, "main_highlight  white/blue  bold");
	    schemer.push(cp_main_frame, "main_frame  blue/blue   bold");
	    schemer.push(cp_main_history_incoming, "main_history_incoming  white/blue");
	    schemer.push(cp_main_history_outgoing, "main_history_outgoing  white/blue  bold");
	    schemer.push(cp_clist_icq, "clist_icq   green/blue");
	    schemer.push(cp_clist_yahoo, "clist_yahoo magenta/blue");
	    schemer.push(cp_clist_msn, "clist_msn   cyan/blue");
	    schemer.push(cp_clist_infocard, "clist_infocard  white/blue");
	    schemer.push(cp_clist_aim, "clist_aim   yellow/blue");
	    schemer.push(cp_clist_irc, "clist_irc   blue/blue   bold");
	    schemer.push(cp_clist_jabber, "clist_jabber    red/blue");
	    schemer.push(cp_clist_rss, "clist_rss    white/blue   bold");
	    schemer.push(cp_clist_lj, "clist_lj    cyan/blue   bold");
	    schemer.push(cp_clist_gadu, "clist_gg    blue/blue   bold");
	    schemer.push(cp_clist_online, "clist_online   green/transparent   bold");
	    schemer.push(cp_clist_offline, "clist_offline   red/transparent   bold");
	    schemer.push(cp_clist_away, "clist_away   yellow/transparent   bold");
	    schemer.push(cp_clist_dnd, "clist_dnd   cyan/transparent   bold");
	    schemer.push(cp_clist_na, "clist_na   blue/transparent   bold");
	    schemer.push(cp_clist_free_for_chat, "clist_free_for_chat   green/transparent   bold");
	    schemer.push(cp_clist_invisible, "clist_invisible   grey/transparent   bold");
	    schemer.push(cp_clist_not_in_list, "clist_not_in_list   white/transparent   bold");
	    break;
    }

    if(access(tname.c_str(), F_OK)) {
	schemer.save(tname);
    } else {
	schemer.load(tname);
    }
}

void icqconf::loadsounds() {
    string tname = getconfigfname("sounds"), buf, suin, skey;
    int n, ffuin, i;
    icqcontact *c;
    imevent::imeventtype it;

    typedef pair<imevent::imeventtype, string> eventsound;
    vector<eventsound> soundnames;
    vector<eventsound>::iterator isn;

    soundnames.push_back(eventsound(imevent::message, "msg"));
    soundnames.push_back(eventsound(imevent::sms, "sms"));
    soundnames.push_back(eventsound(imevent::url, "url"));
    soundnames.push_back(eventsound(imevent::online, "online"));
    soundnames.push_back(eventsound(imevent::offline, "offline"));
    soundnames.push_back(eventsound(imevent::email, "email"));

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	for(it = imevent::message; it != imevent::imeventtype_size; it++) {
	    c->setsound(it, "");
	}
    }

    if(access(tname.c_str(), F_OK)) {
	ofstream fo(tname.c_str());

	if(fo.is_open()) {
	    fo << "# This file contains sound configuration for centericq" << endl;
	    fo << "# Every line should look like: <id> <event> <command>" << endl << "#" << endl;

	    fo << "# <id>\tid of a contact; can be one of the following" << endl;
	    fo << "# *\tmeans default sound for all the contacts" << endl;
	    fo << "# icq_<uin>\tfor icq contacts (e.g. icq_123)" << endl;
	    fo << "# yahoo_<nickname>\tfor yahoo (e.g. yahoo_thekonst)" << endl;
	    fo << "# .. etc. Similar for the other protocols" << endl << "#" << endl;

	    fo << "# <event>\tcan be: ";
	    for(isn = soundnames.begin(); isn != soundnames.end(); ++isn) {
		if(isn != soundnames.begin()) fo << ", ";
		fo << isn->second;
	    }

	    fo << endl << "# <command>\tcommand line to be executed to play a sound" << endl << endl;

	    switch(rs) {
		case rscard:
		    fo << "*\tmsg\tplay " << SHARE_DIR << "/msg.wav" << endl;
		    fo << "*\turl\tplay " << SHARE_DIR << "/url.wav" << endl;
		    fo << "*\temail\tplay " << SHARE_DIR << "/email.wav" << endl;
		    fo << "*\tonline\tplay " << SHARE_DIR << "/online.wav" << endl;
		    fo << "*\toffline\tplay " << SHARE_DIR << "/offline.wav" << endl;
		    fo << "*\tsms\tplay " << SHARE_DIR << "/sms.wav" << endl;
		    break;

		case rsspeaker:
		    fo << "*\tmsg\t!spk1" << endl;
		    fo << "*\turl\t!spk2" << endl;
		    fo << "*\tsms\t!spk3" << endl;
		    fo << "*\temail\t!spk5" << endl;
		    break;
	    }

	    fo.close();
	}
    }

    ifstream fi(tname.c_str());
    if(fi.is_open()) {
	while(!fi.eof()) {
	    getstring(fi, buf);
	    if(buf.empty() || (buf.substr(0, 1) == "#")) continue;

	    suin = getword(buf);
	    skey = getword(buf);

	    it = imevent::imeventtype_size;

	    for(isn = soundnames.begin(); isn != soundnames.end(); ++isn) {
		if(skey == isn->second) {
		    it = isn->first;
		    break;
		}
	    }

	    if(it == imevent::imeventtype_size)
		continue;

	    c = 0;

	    if(suin != "*") {
		if((i = suin.find("_")) != -1) {
		    skey = suin.substr(0, i);
		    suin.erase(0, i+1);

		    imcontact ic;
		    protocolname pname;

		    for(pname = icq; pname != protocolname_size && skey != getprotocolname(pname); pname++);

		    if(pname != protocolname_size) {
			if(suin == "*") {
			    for(i = 0; i < clist.count; i++) {
				c = (icqcontact *) clist.at(i);

				if(c->getdesc().pname == pname) c->setsound(it, buf);
			    }
			} else {
			    if(pname == icq) ic = imcontact(strtoul(suin.c_str(), 0, 0), pname);
				else ic = imcontact(suin, pname);

			    c = clist.get(ic);
			}
		    }
		}
	    } else {
		c = clist.get(contactroot);
	    }

	    if(c) c->setsound(it, buf);
	}

	fi.close();
    }
}

void icqconf::loadactions() {
    string fname = getconfigfname("actions"), buf, name;
    ifstream f;
    bool cont;

    if(access(fname.c_str(), F_OK)) {
	buf = getenv("PATH") ? getenv("PATH") : "";

	ofstream of(fname.c_str());

	if(of.is_open()) {
	    of << "# This file contains external actions configuration for centericq" << endl;
	    of << "# Every line should look like: <action> <command>" << endl;
	    of << "# Possible actions are: openurl, detectmusic" << endl << endl;

	    of << "openurl \\" << endl;
	    of << "    (if test ! -z \"`ps x | grep /" << browser << " | grep -v grep`\"; \\" << endl;
	    of << "\tthen DISPLAY=0:0 " << browser << " -remote 'openURL($url$, new-window)'; \\" << endl;
	    of << "\telse DISPLAY=0:0 " << browser << " \"$url$\"; \\" << endl;
	    of << "    fi) >/dev/null 2>&1 &" << endl;

	    of << "detectmusic \\" << endl;
	    of << "     if test ! -z \"`ps x | grep orpheus | grep -v grep`\"; \\" << endl;
	    of << "\tthen cat ~/.orpheus/currently_playing; \\" << endl;
	    of << "     fi" << endl;

	    of.close();
	}
    }

    f.open(fname.c_str());

    if(f.is_open()) {
	actions.clear();
	cont = false;

	while(!f.eof()) {
	    getstring(f, buf);
	    if(!cont) name = getword(buf);

	    if(name == "openurl" || name == "detectmusic") {
		if(!buf.empty())
		if(cont = buf.substr(buf.size()-1, 1) == "\\")
		    buf.erase(buf.size()-1, 1);

		actions[name] += buf;
	    }
	}

	f.close();
    }
}

icqconf::regcolor icqconf::getregcolor() const {
    return rc;
}

void icqconf::setregcolor(icqconf::regcolor c) {
    rc = c;
}

icqconf::regsound icqconf::getregsound() const {
    return rs;
}

void icqconf::setregsound(icqconf::regsound s) {
    rs = s;
}

icqconf::colormode icqconf::getcolormode() const {
    return cm;
}

void icqconf::setcolormode(colormode c) {
    cm = c;
}

void icqconf::setauto(int away, int na) {
    autoaway = away;
    autona = na;

    if(away == na)
	autoaway = 0;
}

void icqconf::getauto(int &away, int &na) const {
    away = autoaway;
    na = autona;

    if(away == na)
	away = 0;
}

void icqconf::setquote(bool use) {
    quote = use;
}

void icqconf::setsockshost(const string &nsockshost) {
    int pos;

    if(!nsockshost.empty()) {
	if((pos = nsockshost.find(":")) != -1) {
	    sockshost = nsockshost.substr(0, pos);
	    socksport = atol(nsockshost.substr(pos+1).c_str());
	} else {
	    sockshost = nsockshost;
	    socksport = 1080;
	}
    } else {
	sockshost = "";
    }
}

string icqconf::getsockshost() const {
    return sockshost;
}

unsigned int icqconf::getsocksport() const {
    return socksport;
}

void icqconf::getsocksuser(string &name, string &pass) const {
    name = socksuser;
    pass = sockspass;
}

void icqconf::setsocksuser(const string &name, const string &pass) {
    socksuser = name;
    sockspass = pass;
}

void icqconf::setsavepwd(bool ssave) {
    savepwd = ssave;
}

void icqconf::sethideoffline(bool fho) {
    hideoffline = fho;
}

void icqconf::setemacs(bool fem) {
    emacs = fem;
}

void icqconf::setantispam(bool fas) {
    antispam = fas;
}

void icqconf::setmailcheck(bool fmc) {
    mailcheck = fmc;
}

void icqconf::setaskaway(bool faskaway) {
    askaway = faskaway;
}

bool icqconf::getchatmode(protocolname pname) {
    switch(pname) {
	case infocard:
	case rss:
	    return false;
    }

    return chatmode[pname];
}

void icqconf::setchatmode(protocolname pname, bool fchatmode) {
    chatmode[pname] = fchatmode;
}

bool icqconf::getentersends(protocolname pname) {
    return entersends[pname];
}

void icqconf::setentersends(protocolname pname, bool fentersends) {
    entersends[pname] = fentersends;
}

bool icqconf::getnonimonline(protocolname pname) {
    return nonimonline[pname];
}

void icqconf::setnonimonline(protocolname pname, bool fnonimonline) {
    nonimonline[pname] = fnonimonline;
}

bool icqconf::getcpconvert(protocolname pname) const {
    return cpconvert[pname];
}

void icqconf::setcpconvert(protocolname pname, bool fcpconvert) {
    cpconvert[pname] = fcpconvert;
}

string icqconf::execaction(const string &name, const string &param) {
    int inpipe[2], outpipe[2], pid, npos;
    struct sigaction sact, osact;
    string torun = actions[name], out;
    fd_set rfds;
    char ch;

    if(name == "openurl")
    while((npos = torun.find("$url$")) != -1)
	torun.replace(npos, 5, param);

    if(!pipe(inpipe) && !pipe(outpipe)) {
	memset(&sact, 0, sizeof(sact));
	sigaction(SIGCHLD, &sact, &osact);
	pid = fork();

	if(!pid) {
	    dup2(inpipe[1], STDOUT_FILENO);
	    dup2(outpipe[0], STDIN_FILENO);

	    close(inpipe[1]);
	    close(inpipe[0]);
	    close(outpipe[0]);
	    close(outpipe[1]);

	    execl("/bin/sh", "/bin/sh", "-c", torun.c_str(), 0);
	    _exit(0);
	} else {
	    close(outpipe[0]);
	    close(inpipe[1]);

	    while(1) {
		FD_ZERO(&rfds);
		FD_SET(inpipe[0], &rfds);

		if(select(inpipe[0]+1, &rfds, 0, 0, 0) < 0) break; else {
		    if(FD_ISSET(inpipe[0], &rfds)) {
			if(read(inpipe[0], &ch, 1) != 1) break; else {
			    out += ch;
			}
		    }
		}
	    }

	    waitpid(pid, 0, 0);
	    close(inpipe[0]);
	    close(outpipe[1]);
	}

	sigaction(SIGCHLD, &osact, 0);
    }

    face.log(_("+ launched the %s action command"), name.c_str());
    return out;
}

string icqconf::getprotocolname(protocolname pname) const {
    static const string ptextnames[protocolname_size] = {
	"icq", "yahoo", "msn", "aim", "irc", "jab", "rss", "lj", "gg", "infocard"
    };

    return ptextnames[pname];
}

protocolname icqconf::getprotocolbyletter(char letter) const {
    switch(letter) {
	case 'y': return yahoo;
	case 'a': return aim;
	case 'i': return irc;
	case 'm': return msn;
	case 'n': return infocard;
	case 'j': return jabber;
	case 'r': return rss;
	case 'l': return livejournal;
	case 'g': return gadu;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': return icq;
    }

    return protocolname_size;
}

string icqconf::getprotocolprefix(protocolname pname) const {
    static const string pprefixes[protocolname_size] = {
	"", "y", "m", "a", "i", "j", "r", "l", "g", "n"
    };

    return pprefixes[pname];
}

imstatus icqconf::getstatus(protocolname pname) {
    if(startoffline)
	return offline;

    imstatus st = available;
    map<string, string>::iterator ia;
    imaccount a = getourid(pname);

    if((ia = a.additional.find("status")) != a.additional.end()) {
	if(!ia->second.empty()) {
	    for(st = offline; st != imstatus_size && imstatus2char[st] != ia->second[0]; st++);
	    if(st == imstatus_size) st = available;
	}
    }

    return st;
}

void icqconf::savestatus(protocolname pname, imstatus st) {
    imaccount im = getourid(pname);
    im.additional["status"] = string(1, imstatus2char[st]);
    setourid(im);
}

int icqconf::getprotcolor(protocolname pname) const {
    switch(pname) {
	case icq : return getcolor(cp_clist_icq);
	case yahoo : return getcolor(cp_clist_yahoo);
	case infocard : return getcolor(cp_clist_infocard);
	case msn : return getcolor(cp_clist_msn);
	case aim : return getcolor(cp_clist_aim);
	case irc : return getcolor(cp_clist_irc);
	case jabber : return getcolor(cp_clist_jabber);
	case rss : return getcolor(cp_clist_rss);
	case livejournal : return getcolor(cp_clist_lj);
	case gadu : return getcolor(cp_clist_gadu);
	default : return getcolor(cp_main_text);
    }
}

int icqconf::getstatuscolor(imstatus status) const {
    switch(status) {
        case offline : return getcolor(cp_clist_offline);
        case available : return getcolor(cp_clist_online);
        case invisible : return getcolor(cp_clist_invisible);
        case freeforchat : return getcolor(cp_clist_free_for_chat);
        case dontdisturb : return getcolor(cp_clist_dnd);
        case occupied : return getcolor(cp_clist_occupied);
        case notavail : return getcolor(cp_clist_na);
        case away : return getcolor(cp_clist_away);
	default : return getcolor(cp_main_text);
    }
}

void icqconf::setbidi(bool fbidi) {
    bidi = fbidi;
#ifdef USE_FRIBIDI
    use_fribidi = bidi;
#endif
}

void icqconf::commandline(int argc, char **argv) {
    int i;
    string event, proto, dest, number;
    char st = 0;

    argv0 = argv[0];

    for(i = 1; i < argc; i++) {
	string args = argv[i];

	if((args == "-a") || (args == "--ascii")) {
	    kintf_graph = 0;

	} else if((args == "-b") || (args == "--basedir")) {
	    if(argv[++i]) {
		basedir = argv[i];
		if(basedir.substr(basedir.size()-1) != "/") basedir += "/";
	    }

	} else if((args == "-B") || (args == "--bind")) {
	    if(argv[++i]) {
		bindhost = argv[i];
		cw_setbind(bindhost.c_str());
	    }

	} else if((args == "-T") || (args == "--no-xtitles")) {
	    notitles = true;

	} else if((args == "-o") || (args == "--offline")) {
	    startoffline = true;

	} else if((args == "-s") || (args == "--send")) {
	    if(argv[++i]) event = argv[i];

	} else if((args == "-p") || (args == "--proto")) {
	    if(argv[++i]) proto = argv[i];

	} else if((args == "-t") || (args == "--to")) {
	    if(argv[++i]) dest = argv[i];

	} else if((args == "-n") || (args == "--number")) {
	    if(argv[++i]) number = argv[i];

	} else if((args == "-S") || (args == "--status")) {
	    if(argv[++i])
	    if(strlen(argv[i]))
		st = argv[i][0];

	} else if((args == "-d") || (args == "--debug")) {
	    debug = true;

	} else if((args == "-v") || (args == "--version")) {
	    cout << PACKAGE << " " << VERSION << endl
		<< "Written by Konstantin Klyagin." << endl
		<< "Built-in protocols are:";

	    for(protocolname pname = icq; pname != protocolname_size; pname++)
		if(gethook(pname).enabled()) cout << " " << conf.getprotocolname(pname);

	    cout << endl << endl
		<< "This is free software; see the source for copying conditions.  There is NO" << endl
		<< "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." << endl;

	    exit(0);

	} else {
	    usage();
	    exit(0);
	}
    }

    if(!number.empty()) {
	if(event.empty()) event = "sms";
	if(proto.empty()) proto = "icq";
	dest = "0";
    }

    constructevent(event, proto, dest, number);
    externalstatuschange(st, proto);
}

void icqconf::constructevent(const string &event, const string &proto,
const string &dest, const string &number) const {

    imevent *ev = 0;
    imcontact cdest;
    string text, buf;
    int pos;

    if(event.empty() && dest.empty()) return; else
    if(event.empty() || proto.empty() || dest.empty()) {
	cout << _("event sending error: not enough parameters") << endl;
	exit(1);
    }

    while(getline(cin, buf)) {
	if(!text.empty()) text += "\n";
	text += buf;
    }

    if(proto == "icq") {
	if(dest.find_first_not_of("0123456789") != -1) {
	    cout << _("event sending error: only UINs are allowed with icq protocol") << endl;
	    exit(1);
	}
	cdest = imcontact(strtoul(dest.c_str(), 0, 0), icq);
    } else {
	protocolname pname;

	for(pname = icq; pname != protocolname_size; pname++) {
	    if(getprotocolname(pname) == proto) {
		cdest = imcontact(dest, pname);
		break;
	    }
	}

	if(pname == protocolname_size) {
	    cout << _("event sending error: unknown IM type") << endl;
	    exit(1);
	}
    }

    if(event == "msg") {
	ev = new immessage(cdest, imevent::outgoing, text);
    } else if(event == "url") {
	string url;

	if((pos = text.find("\n")) != -1) {
	    url = text.substr(0, pos);
	    text.erase(0, pos+1);
	} else {
	    url = text;
	    text = "";
	}

	ev = new imurl(cdest, imevent::outgoing, url, text);
    } else if(event == "sms") {
	ev = new imsms(cdest, imevent::outgoing, text, number);
    } else {
	cout << _("event sending error: unknown event type") << endl;
	exit(1);
    }

    if(ev) {
	icqcontact *c = new icqcontact(cdest);

	if(access(c->getdirname().c_str(), X_OK)) {
	    mkdir(c->getdirname().c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
	    selfsignal(SIGUSR2);
	}

	if(!access(c->getdirname().c_str(), X_OK)) {
	    clist.add(c);
	    em.store(*ev);

	    char buf[512];
	    sprintf(buf, _("%s to %s has been put to the queue"),
		streventname(ev->gettype()),
		number.empty() ? c->getdesc().totext().c_str() : number.c_str());

	    cout << buf << endl;
	} else {
	    cout << _("event sending error: error creating directory for the contact") << endl;
	    exit(1);
	}

	//delete c;
	delete ev;

	exit(0);
    }
}

void icqconf::selfsignal(int signum) const {
    int rpid;
    ifstream f(conf.getconfigfname("pid").c_str());

    if(f.is_open()) {
	f >> rpid;
	f.close();

	if(rpid > 0) {
	    kill(rpid, signum);
	}
    }
}

void icqconf::externalstatuschange(char st, const string &proto) const {
    imstatus imst;
    protocolname pname;

    if(st) {
	for(pname = icq; pname != protocolname_size; pname++)
	    if(getprotocolname(pname) == proto)
		break;

	for(imst = offline; imst != imstatus_size; imst++)
	    if(imstatus2char[imst] == st)
		break;

	try {
	    if(pname != protocolname_size) {
		if(imst != imstatus_size) {
		    string sfname = conf.getconfigfname((string) "status-" + proto);
		    ofstream sf(sfname.c_str());
		    if(sf.is_open()) sf << imstatus2char[imst], sf.close();

		    selfsignal(SIGUSR1);
		} else {
		    throw _("unknown status character was given");
		}
	    } else {
		throw _("unknown IM type");
	    }
	} catch(char *p) {
	    cout << _("status change error: ") << p << endl;
	    exit(1);
	}

	exit(0);
    }
}

void icqconf::usage() const {
    cout << _("Usage: ") << argv0 << " [OPTION].." << endl;

    cout << endl << _("General options:") << endl;
    cout << _("  --ascii, -a              use ASCII characters for windows and UI controls") << endl;
    cout << _("  --basedir, -b <path>     set a custom base directory") << endl;
    cout << _("  --bind, -B <host/ip>     bind a custom local IP") << endl;
    cout << _("  --no-xtitles, -T         disable xterm titles") << endl;
    cout << _("  --offline, -o            set all protocols status to offline upon start") << endl;
    cout << _("  --debug, -d              enables debug info logging") << endl;
    cout << _("  --help                   display this stuff") << endl;
    cout << _("  --version, -v            show the program version info") << endl;

    cout << endl << _("Events sending options:") << endl;
    cout << _("  -s, --send <event type>  event type; can be msg, sms or url") << endl;
    cout << _("  -S, --status <status>    change the current IM status") << endl;
    cout << _("  -p, --proto <protocol>   protocol type; can be icq, yahoo, msn, aim, irc, jab, rss, lj, gg or infocard") << endl;

    cout << _("  -t, --to <destination>   destination UIN or nick (depends on protocol)") << endl;
    cout << _("  -n, --number <phone#>    mobile number to send an event to (sms only)") << endl;

    cout << endl << _("Report bugs to <centericq-bugs@konst.org.ua>.") << endl;
}

void icqconf::setproxy() {
    if(getproxyconnect()) {
	cw_setproxy(gethttpproxyhost().c_str(), gethttpproxyport(),
	    gethttpproxyuser().c_str(),
	    gethttpproxypasswd().c_str());

    } else {
	cw_setproxy(0, 0, 0, 0);

    }
}

void icqconf::setmakelog(bool slog) {
    makelog = slog;
}

void icqconf::setproxyconnect(bool sproxy) {
    proxyconnect = sproxy;
    setproxy();
}

void icqconf::setproxyssl(bool sproxyssl) {
    proxyssl = sproxyssl;
}

void icqconf::setgroupmode(icqconf::groupmode amode) {
    fgroupmode = amode;
}

void icqconf::initmultiproto(bool p[], string buf, bool excludenochat) {
    string w;
    protocolname pname;

    for(pname = icq; pname != protocolname_size; pname++)
	p[pname] = buf.empty();

    while(!(w = getword(buf)).empty()) {
	for(pname = icq; pname != protocolname_size; pname++) {
	    if(getprotocolname(pname) == w) {
		if(excludenochat) {
		    p[pname] = !gethook(pname).getCapabs().count(hookcapab::nochat);
		} else {
		    p[pname] = true;
		}
		break;
	    }
	}
    }
}

void icqconf::setbrowser(const string &abrowser) {
  if (!abrowser.empty())
    browser = abrowser;
  else
    browser = "mozilla";
}

void icqconf::setsmtphost(const string &asmtphost) {
    int pos;

    smtphost = "";
    smtpport = 0;

    if(!asmtphost.empty()) {
	if((pos = asmtphost.find(":")) != -1) {
	    smtphost = asmtphost.substr(0, pos);
	    smtpport = atol(asmtphost.substr(pos+1).c_str());
	} else {
	    smtphost = asmtphost;
	    smtpport = 25;
	}
    }

    if(smtphost.empty() || !smtpport) {
	smtphost = getsmtphost();
	smtpport = getsmtpport();
    }
}

void icqconf::sethttpproxyhost(const string &ahttpproxyhost) {
    int pos, posn;

    httpproxyhost = "";
    httpproxyport = 0;
    httpproxyuser = "";
    httpproxypasswd = "";

    if(!ahttpproxyhost.empty()) {
	if((pos = ahttpproxyhost.find(":")) != -1) {
	    if ((posn = ahttpproxyhost.find("@")) != -1) {
		httpproxyuser = ahttpproxyhost.substr(0, pos);
		httpproxypasswd = ahttpproxyhost.substr(pos+1, posn-pos-1);
		if((pos = ahttpproxyhost.find(":", posn)) != -1) {
		    httpproxyhost = ahttpproxyhost.substr(posn+1, pos-posn-1);
		    httpproxyport = atol(ahttpproxyhost.substr(pos+1).c_str());
		}
	    } else {
		httpproxyhost = ahttpproxyhost.substr(0, pos);
		httpproxyport = atol(ahttpproxyhost.substr(pos+1).c_str());
	    }
	} else {
	    httpproxyhost = ahttpproxyhost;
	    httpproxyport = 8080;
	}
    }

    setproxy();
}

string icqconf::getbrowser() const {
  return browser.empty() ? "mozilla" : browser;
}

string icqconf::getsmtphost() const {
    return smtphost.empty() ? "localhost" : smtphost;
}

unsigned int icqconf::getsmtpport() const {
    return smtpport ? smtpport : 25;
}

string icqconf::gethttpproxyhost() const {
    return httpproxyhost;
}

unsigned int icqconf::gethttpproxyport() const {
    return httpproxyport ? httpproxyport : 8080;
}

string icqconf::gethttpproxyuser() const {
    return httpproxyuser;
}

string icqconf::gethttpproxypasswd() const {
    return httpproxypasswd;
}

void icqconf::checkdiskspace() {
    fenoughdiskspace = true;

#ifndef __sun__
    struct statfs st;
    if(!statfs(conf.getdirname().c_str(), &st)) {
#else
    struct statvfs st;
    if(!statvfs(conf.getdirname().c_str(), &st)) {
#endif
	fenoughdiskspace = ((double) st.f_bavail) * st.f_bsize >= 10240;
    }
}

void icqconf::setcharsets(const string &from, const string &to) {
    fromcharset = from;
    tocharset = to;
}

const char *icqconf::getconvertfrom(protocolname pname) const {
    if(pname != protocolname_size && !getcpconvert(pname)) return "iso-8859-1";
    return fromcharset.c_str();
}

const char *icqconf::getconvertto(protocolname pname) const {
    if(pname != protocolname_size && !getcpconvert(pname)) return "iso-8859-1";
    return tocharset.c_str();
}

// ----------------------------------------------------------------------------

icqconf::imaccount::imaccount() {
    uin = port = 0;
    pname = infocard;
}

icqconf::imaccount::imaccount(protocolname apname) {
    uin = port = 0;
    pname = apname;
}

bool icqconf::imaccount::empty() const {
    return !uin && nickname.empty();
}

void icqconf::imaccount::write(ofstream &f) {
    string prefix = conf.getprotocolname(pname) + "_";
    map<string, string>::iterator ia;

    f << endl;
    if(!nickname.empty()) f << prefix << "nick\t" << nickname << endl;
    if(uin) f << prefix << "uin\t" << uin << endl;
    if(conf.getsavepwd()) f << prefix << "pass\t" << password << endl;
    if(!server.empty()) {
	f << prefix << "server\t" << server;
	if(port) f << ":" << port;
	f << endl;
    }

    for(ia = additional.begin(); ia != additional.end(); ++ia)
	if(!ia->first.empty() && !ia->second.empty())
	    f << prefix << ia->first << "\t" << ia->second << endl;
}

void icqconf::imaccount::read(const string &spec) {
    int pos = spec.find("_");
    string spname, buf;

    if(pos != -1) {
	buf = spec;
	spname = getword(buf.erase(0, pos+1));

	if(spname == "nick") nickname = buf; else
	if(spname == "uin") uin = strtoul(buf.c_str(), 0, 0); else
	if(spname == "pass") password = buf; else
	if(spname == "server") {
	    if((pos = buf.find(":")) != -1) {
		server = buf.substr(0, pos);
		port = strtoul(buf.substr(pos+1).c_str(), 0, 0);
	    } else {
		server = buf;
	    }

	    if(pname == icq)
	    if(server == "icq.mirabilis.com")
		server = "";

	    if(pname == yahoo)
	    if(server == "scs.yahoo.com")
		server = "";

	} else {
	    additional[spname] = buf;
	}
    }
}

bool icqconf::imaccount::operator == (protocolname apname) const {
    return pname == apname;
}

bool icqconf::imaccount::operator != (protocolname apname) const {
    return pname != apname;
}
