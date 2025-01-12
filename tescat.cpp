#include <emscripten.h>
#include <vector>
#include <map>
#include <string>
#include <time.h>
#include <cctype>

using namespace std;

struct tesdata {
  int type;
  const char *fname;
  const char *label;
  const char *desc;
  int kind;
  const char *link;
  };

vector<tesdata> alldata = {
#include "table.cpp"
#include "table-arcm.cpp"
#include "table-upto5.cpp"
};

string imglink(string lnk) {
  return "<img src=\"https://images2.imgbox.com/" + lnk + "_o.png\"/>";
  }

const int ANY = -2;
int curreq = ANY;

void set_value(string name, string s) {
  EM_ASM_({
    var name = UTF8ToString($0, $1);
    var value = UTF8ToString($2, $3);
    document.getElementById(name).innerHTML = value;
    }, name.c_str(), int(name.size()),
    s.c_str(), int(s.size())
    );
  }

void set_location(string s) {
  EM_ASM_({
    var value = UTF8ToString($0, $1);
    window.history.pushState(value, document.title + ":"+value, "?c=" + encodeURIComponent(value));
    }, s.c_str(), int(s.size())
    );
  }

string genlink(string to, string cap) {
  // return "<a href=\"./?c=" + to + "\">" + cap + "</a>";
  return "<a href=\"javascript:jump('" + to + "')\">" + cap + "</a>";
  }

vector<tuple<string, string, int> > restrictions = {
  {"", "any", ANY}, {"H/", "hyperbolic", -1}, {"E/", "Euclidean", 0}, {"S/", "spherical", 1}
  };

int numstrcmp(const char *a, const char *b) {
  if(isdigit(*a) && isdigit(*b)) {
    int va = 0, vb = 0;
    while(isdigit(*a)) va = 10*va + *(a++) - '0';
    while(isdigit(*b)) vb = 10*vb + *(b++) - '0';
    if(va != vb) return va - vb;
    return numstrcmp(a, b);
    }
  else if(*a != *b) return *a - *b;
  else if(*a == 0) return 0;
  else return numstrcmp(a+1, b+1);
  }

string seekstr;

bool at(const char *fname, const string& needle) {
  for(char c: needle) if(*(fname++) != c) return false;
  return true;
  }

void generate_page(string s) {
  set_location(s);
  srand(time(NULL));  
  string out;
  string parent;
  
  sort(alldata.begin(), alldata.end(), [] (const tesdata& d1, const tesdata& d2) { return numstrcmp(d1.fname, d2.fname) < 0; });
  
  string prefix;
  curreq = ANY;
  
  
  // string advsearch, advsearch_str;
  
  for(auto&[code, full, val]: restrictions) {
    if(code != "" && s.substr(0, 2) == code) {
      curreq = val;
      s = s.substr(2);
      prefix = code;
      }
    }
  
  /*
  if(s.substr(0, 7) == "search/") {
    int i = 7;
    while(i < int(s.size()-1) && (s[i] != '/' || s[i+1] != '/')) i++;
    advsearch = s.substr(7, i-7);
    advsearch_str = "search/" + advsearch + "//";
    if(i <= int(s.size() - 2))
      s = s.substr(i+2);
    }
  */

  while(s != "" && s[0] == '/') s = s.substr(1);
  
  if(s != "") {
    for(int i=0; i<int(s.size())-1; i++) if(s[i] == '/') parent = s.substr(0, i+1);
    out += genlink(prefix+/*advsearch_str+*/parent, "go back!") + "<br/><br/>";
    }
  
  int len = s.size();
  
  vector<tesdata*> matching;
  matching.reserve(alldata.size());

  for(auto& td: alldata) {
    if(td.type != curreq && curreq != ANY) continue;
    if(at(td.fname, s)) 
      matching.push_back(&td);
    }

  if(s == "") {
    out += "An online version of the catalog of tessellations, compiled by Marek Čtrnáct.<br/><br/>";
    out += "<ul>\n"
      "<li> Links run the tessellation in the <a href=\"http://roguetemple.com/z/hyper/\">HyperRogue engine</a>, where you can view it from other angles, or make a "
      "high quality SVG screenshot (press shift+A)."
      "<li> All the tessellation files can be downloaded <a href=\"https://github.com/zenorogue/tes-catalog/releases\">here</a>."
      "<li> In Archimedean tilings, the notation like \"A:(4,4); B:(4,4); \"(B,B,A,A,A) (1)(4)\" means that A and B can get any value which is at least 4 and divisible by 4.";

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    if(tm.tm_mon == 5 && tm.tm_mday == 17)
      out += "<li> <b>Happy World Tessellation Day!</b><br/>";
    
    out +=
      "<li> Join the <a href=\"https://discord.gg/SnuhaW8\">#tessellation channel on the HyperRogue discord</a> for discussions!<br/>"
      "<li> See <a href=\"https://github.com/zenorogue/tes-catalog\">here</a> for raw tessellation files, the source code of this website, and citation info!<br/>"
      "</ul><br/><br/>";
    }

  out += "Curvature:";
  bool next = false;
  string current;
  for(auto& [code, full, val]: restrictions) {
    if(next) out += " | ";
    next = true;
    out += genlink(code + seekstr + s, full);
    if(val == curreq) current = full;
    }
  out += " (currently showing: " + current + ")<br/><br/>";

  // out += "Displaying advanced search results for: '" + advsearch + "'<br/><br/>";

  out += "Advanced search: <input id=\"search\" type=\"text\"><button onClick=\"do_advanced_search('" + prefix + "', '" + s + "')\">Search!</button><br/><br/>";
    
  out += "Number of tessellations here: " + std::to_string(matching.size()) + "<br/><br/>";    
          
  string last_explored = "?";
  
  for(int ti=0; ti<(int) matching.size(); ti++) {
    auto& td = *matching[ti];
    const string& fname = td.fname;
    const string& label = td.label;
    const string& desc = td.desc;

    for(int i=len; i<int(fname.size()); i++) if(fname[i] == '/') {
      string where = fname.substr(0, i+1);
      if(true) {
        int wlen = where.size();
        map<string, pair<int, string> > samples;
        vector<string> justhere;
        int cnt = 0;
        
        while(ti < (int) matching.size()) {
          auto& td1 = *matching[ti];
          if(!at(td1.fname, where)) break;
          cnt++;
          const string& fname1 = td1.fname;
          for(int i=wlen; i<int(fname1.size()); i++) if(fname1[i] == '/') {
            auto& sa = samples[fname1.substr(wlen, i-wlen)];
            sa.first++;
            if(rand() % sa.first == 0) sa.second = td1.link;
            goto next_td1;
            }
          justhere.push_back(td1.link);
          next_td1: ti++;
          }
        ti--;
        
        for(auto sa: samples) justhere.push_back(sa.second.second);
        
        string images = "";
        int left = 10;
        int n = justhere.size();
        images += "<table><tr>";
        for(auto j: justhere) {
          if(rand() % n < left) {
            images += "<td>&nbsp;" + imglink(j) + "&nbsp;</td>";
            left--;
            if(left == 5) images += "</tr><tr>";
            }
          n--;
          }
        images += "</tr></table>";

        out += "<h2>" + genlink(prefix+/*advsearch_str+*/where, where) + " (" + std::to_string(cnt) + " tessellations)</h2>";
        out += genlink(prefix+where, images);
        out += "<br/><br/>";
        }
      goto next_td;
      }
    
    if(1) {
      string cline = 
        "view.html?c=-viz";
      
      cline += "+-back+ffffff+-fore+0+-borders+ffffff+-fillmodel+ff";
      
      if(td.type >= 0) 
        cline += "+-zoom+.95";

      // cline += "+-wsh+9+-palrgba+sub+00000020+-palrgba+normal+000000FF+-smart+1";
      cline += "+-smart+1";
      
      if(td.kind == 1) 
        cline += "+-canvas+B";
      else
        cline += "+-canvas+A";
        
      if(td.kind == 0) {      
        cline += "+-arbi+1&1=tessellations%2F";
        for(char ch:fname)
          if(ch == '+') cline += "%2B";
          else if(ch == '/') cline += "%2F";
          else cline += ch;
        cline += ".tes";
        }
      else {
        cline += "+-7+-symbol+\"";
        string lab = td.label;
        for(char c: lab)
          if(c == '(') cline += "%28";
          else if(c == ')') cline += "%29";
          else if(c == '[') cline += "%5B";
          else if(c == ']') cline += "%5D";
          else if(c == ',') cline += "%2C";
          else if(c == ' ') cline += "%2C";
          else cline += c;
        cline += "\"";
        }
  
      char buf[9999];
      snprintf(buf, 9999, "<table><tr><td>%s</td><td><b>%s</b> <a href='%s'>(play online)</a>",
        imglink(td.link).c_str(),
        label.c_str(),
        cline.c_str());
      out += buf;
      
      if(td.kind == 0) {
        snprintf(buf, 9999, "<a href=\"%s\">(download)</a>",("files/tessellations/" + fname + ".tes").c_str());
        out += buf;
        }
      else out += " (Archimedean)";
      
      snprintf(buf, 9999, "<br/><br/>%s</td></tr></table>\n",
        desc.c_str()
        );
      out += buf;
      }
    
    next_td: ;
    }

  set_value("all", out);
  }

extern "C" {
  void doit(const char *s) {
    generate_page(s);
    }
  }

