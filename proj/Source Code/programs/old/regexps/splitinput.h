typedef struct split_name
{
  char *firstname;
  char *mi;
  char *lastname;
  char *endptr;
} split_name;

typedef struct split_date
{
  int month;
  int day;
  int year;
  char *endptr;
} split_date;

typedef struct split_time
{
  int hour;
  int minute;
  int second;
  int millisecond;
  char *endptr;
} split_time;

typedef struct split_telephone
{
  char *areacode;
  char *number;
  char *endptr;
} split_telephone;

typedef struct split_currency
{
  double money;
  char *endptr;
} split_currency;

typedef struct split_int
{
  int value;
  char *endptr;
} split_int;

typedef struct split_email
 {
  char *email;
  char *hostname;
  char *endptr;
} split_email;

typedef struct split_zipcode
{
  char *zip;
  char *plus4;
  char *endptr;
} split_zipcode;


split_name *splitname(char *);
split_date *splitdate(char *);
split_time *splittime(char *);
split_telephone *splittelephone(char *);
split_currency *splitcurrency(char *);
split_email *splitemail(char *);
split_zipcode *splitzipcode(char *);
split_int *splitint(char *);
