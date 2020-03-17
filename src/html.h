#include <avr/pgmspace.h>

const char TXT_CONTENT_TYPE_TEXT_HTML[] PROGMEM = "text/html; charset=utf-8";
const char TXT_CONTENT_TYPE_TEXT_PLAIN[] PROGMEM = "text/plain";

const char WEB_PAGE_HEADER[] PROGMEM = "<!DOCTYPE html><html>\
<head>\
<title>{title}</title>";

const char WEB_PAGE_HEADER_HEAD[] PROGMEM = "<meta name = 'viewport' content = 'width=600' />\
<style>\
body\
{\
    background-color : #EDEDED;\
    font-family: Arial, Helvetica, Sans-Serif;\
    color: #333;\
}\
\
h1 {\
  background-color: #333;\
  display: table-cell;\
  margin: 20px;\
  padding: 20px;\
  color: white;\
  border-radius: 10px 10px 0 0;\
  font-size: 20px;\
}\
\
ul {\
  list-style-type: none;\
  margin: 0;\
  padding: 0;\
  overflow: hidden;\
  background-color: #333;\
  border-radius: 0 10px 10px 10px;}\
\
li {\
  float: left;\
}\
\
li a {\
  display: block;\
  color: #FFF;\
  text-align: center;\
  padding: 16px;\
  text-decoration: none;\
}\
\
li a:hover {\
  background-color: #111;\
}\
\
#main {\
  padding: 20px;\
  background-color: #FFF;\
  border-radius: 10px;\
  margin: 10px 0;\
}\
\
#footer {\
  border-radius: 10px;\
  background-color: #333;\
  padding: 10px;\
  color: #FFF;\
  font-size: 12px;\
  text-align: center;\
}\
table  {\
border-spacing: 0;\
}\
table td, table th {\
padding: 5px;\
}\
table tr:nth-child(even) {\
background: #EDEDED;\
}input[type='submit'] {\
background-color: #333;\
border: none;\
color: white;\
padding: 5px 25px;\
text-align: center;\
text-decoration: none;\
display: inline-block;\
font-size: 16px;\
margin: 4px 2px;\
cursor: pointer;\
}\
input[type='submit']:hover {\
background-color:#4e4e4e;\
}\
input[type='submit']:disabled {\
opacity: 0.6;\
cursor: not-allowed;\
}\
</style>\
</head>";

const char WEB_PAGE_BODY[] PROGMEM = "<body>\
<h1>{title}</h1>\
<ul>\
<li><a href='/'>Home</a></li>\
<li><a href='/switch'>Switch</a></li>\
<li><a href='/settings'>Settings</a></li>\
<li><a href='/wifiscan'>WiFi Scan</a></li>\
<li><a href='/fwupdate'>FW Update</a></li>\
<li><a href='/reboot'>Reboot</a></li>\
</ul>\
<div id='main'>";

const char WEB_PAGE_FOOTER[] PROGMEM = "</div><div id='footer'>&copy; 2020 Fabian Otto - Firmware v{firmware} - Compiled at {compiled}</div>\
</body>\
</html>";

const char WEB_PAGE_NOTFOUND[] PROGMEM = "URI: {uri}<br />\n\
Method: {methode}<br />\n\
Arguments: {args}";

const char WEB_IOS_REDIRECT[] PROGMEM = "<html><body>Redirecting...\
<script type=\"text/javascript\">\
window.location = \"http://192.168.4.1/config\";\
</script>\
</body></html>";
