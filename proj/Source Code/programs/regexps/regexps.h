#define NAME_R		"(([A-Z][A-Za-z']+), ([A-Z][A-Za-z']+)( ([A-Z])\\.?)?)" \
			"|(([A-Z][A-Za-z']+)( ([A-Z])\\.?)? ([A-Z][A-Za-z']+))"
/*
 * Full name: Last name, first name, (middle initial)
 */
#define LNFNMI_R	"([A-Z][A-Za-z']+), ([A-Z][A-Za-z']+)( ([A-Z])\\.?)?"
/*
 * Full name: First name, (middle initial), lastname
 */
#define FNMILN_R	"([A-Z][A-Za-z']+)( ([A-Z])\\.?)? ([A-Z][A-Za-z']+)"
/*
 * Zipcode: 5 digits, (4 digits)
 */
#define ZIPCODE_R	"(\\d{5})(-(\\d{4}))?"
/*
 * Currency: (dollar sign), some digits, 2 digits
 */
#define CURRENCY_R	"\\$?(\\d+\\.\\d{2})"
/*
 * Time: hours, minutes, (seconds, (milliseconds))
 */
#define TIME_R		"(0[1-9]|1[0-2]):([0-5]\\d)(:([0-5]\\d)(\\.(\\d{1,4}))?)?"
/*
 * Date: month, day, (century), year
 */
#define DATE1_R		"(0[1-9]|1[0-2])/(0[1-9]|[12]\\d|3[01])" \
			"/(\\d{2,})"
/*
 * Date: (century), year, month, day
 */
#define DATE2_R		"(\\d{2,})-(0[1-9]|1[0-2])" \
			"-(0[1-9]|[12]\\d|3[01])"
/*
 * Telephone: (area code), 3 digits, 4 digits
 */
#define TELEPHONE1_R	"(\\(([1-9]\\d{2})\\)\\s?|([1-9]\\d{2})-)?" \
			"(\\d{3}-\\d{4})"
#define TELEPHONE2_R	"([1-9]\\d{2})?(\\d{7})"
/*
 * Integer: (sign), some digits
 */
#define INTEGER_R	"[\\-+]?\\d+"
/*
 * Email: recipient, (host), domain, tld
 */
#define EMAIL_R		"(\\w[\\w\\.]*)@((\\w+\\.)+\\w+)"
