#include <string.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pcre.h>
#include "splitinput.h"
#include "regexps.h"

#define VECSIZE 48
split_currency *splitcurrency(char *curr)
{
  int nvecs, *vector;
  char *buffer;
  struct split_currency *sc=NULL;
  static int r_erroffset;
  static const char *r_errptr;
  static pcre *currency_code = NULL;

    
 /*
  * If we haven't been called yet, compile the regexp.
  */
     
  if (!currency_code) {
    if (!(currency_code = pcre_compile(CURRENCY_R,0,&r_errptr,
						&r_erroffset,NULL))) {
      printf("Pattern 1 failed to compile.\n%d: %s\n",r_erroffset,r_errptr);
      return NULL;
    }
  }

 /*  
  * Allocate the vector for the regex.
  */

  if (!(vector = malloc(sizeof(int) * VECSIZE))) {
    return NULL;
  }
    
 /*   
  * Test the string against the compiled regex.
  */
      
  if ((nvecs = pcre_exec(currency_code,NULL,curr,strlen(curr),
			 0,0,vector,VECSIZE)) >= 0) {
      
   /*
    * Allocate the struct for the split currency.
    */
      
    if (!(buffer = malloc(1 + vector[1] - vector[0]))) {
      free(vector);
      return NULL;
    }
    if (!(sc = malloc(sizeof(struct split_currency)))) {
      free(buffer);
      free(vector);
      return NULL;
    }
 
   /*
    * If the regex covers the whole string, set the end pointer to NULL.
    * Otherwise, point it to the character following the matched string.
    */

    if (vector[1] == strlen(curr)) {
      sc->endptr = NULL;
    } else {
      sc->endptr = curr + vector[1];
    } 
     
   /*
    * Change the string to a double.
    */

    strncpy(buffer,curr + vector[2],1 + vector[1] - vector[0]);
    sc->money = strtod(buffer,NULL);
    free(buffer);
  }
  free(vector);
  return sc;
}

split_date *splitdate(char *dt)
{
  int yr, nvecs, *vector;
  char *buffer;
  struct split_date *sd = NULL;
  static int r_erroffset;
  static const char *r_errptr;
  static pcre *date1_code = NULL,
	      *date2_code = NULL;
  static int days[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

 /*
  * If we haven't been called yet, compile the regexps.
  */

  if (!date1_code) {
    if (!(date1_code = pcre_compile(DATE1_R,0,&r_errptr,&r_erroffset,NULL))) {
      printf("Pattern 1 failed to compile.\n%d: %s\n",r_erroffset,r_errptr);
      return NULL;
    }
    if (!(date2_code = pcre_compile(DATE2_R,0,&r_errptr,&r_erroffset,NULL))) {
      printf("Pattern 2 failed to compile.\n%d: %s\n",r_erroffset,r_errptr);
      return NULL;
    }
  }

 /*
  * Allocate the vector for the regex.
  */

  if (!(vector = malloc(sizeof(int) * VECSIZE))) {
    return NULL;
  }

 /*
  * Test the string against the compiled regex.
  */

  if ((nvecs = pcre_exec(date1_code,NULL,dt,strlen(dt),
			 0,0,vector,VECSIZE)) >= 0) {

   /*
    * Allocate the struct for the split date.
    */

    if (!(sd = malloc(sizeof(struct split_date)))) {
      free(vector);
      return NULL;
    }

   /*
    * If the regex covers the whole string, set the end pointer to NULL.
    * Otherwise, point it to the character following the matched string.
    */

    if (vector[1] == strlen(dt)) {
      sd->endptr = NULL;
    } else {
      sd->endptr = dt + vector[1];
    }

   /*
    * Compute the month, day and year values.
    */

    sd->month = strtol(dt + vector[2], NULL, 10);
    sd->day = strtol(dt + vector[4], NULL, 10);
    yr = sd->year = strtol(dt + vector[6], NULL, 10);

   /*
    * Test for leap year.  If it is, change the days in Feb. to 29.
    */

    if (((yr % 4) == 0)  && (((yr % 100) == 0) == ((yr % 400) == 0))) {
      days[2] = 29;
    }

   /*
    * If the day value is not in the proper range, destroy the structure
    * and return NULL.
    */

    if (sd->day > days[sd->month]) {
      free(sd);
      sd = NULL;
    }

 /*
  * Test the string against the compiled regex.
  */

  } else if ((nvecs = pcre_exec(date2_code,NULL,dt,strlen(dt),
				0,0,vector,VECSIZE)) >= 0) {

   /*
    * Allocate the struct for the split date.
    */

    if (!(sd = malloc(sizeof(struct split_date)))) {
      free(vector);
      return NULL;
    }

   /*
    * If the regex covers the whole string, set the end pointer to NULL.
    * Otherwise, point it to the character following the matched string.
    */

    if (vector[1] == strlen(dt)) {
      sd->endptr = NULL;
    } else {
      sd->endptr = dt + vector[1];
    }

   /*
    * Compute the month, day and year values.
    */

    sd->month = strtol(dt + vector[4], NULL, 10);
    sd->day = strtol(dt + vector[6], NULL, 10);
    yr = sd->year = strtol(dt + vector[2], NULL, 10);

   /*
    * Test for leap year.  If it is, change the days in Feb. to 29.
    */

    if (((yr % 4) == 0)  && (((yr % 100) == 0) == ((yr % 400) == 0))) {
      days[2] = 29;
    }

   /*
    * If the day value is not in the proper range, destroy the structure
    * and return NULL.
    */

    if (sd->day > days[sd->month]) {
      free(sd);
      sd = NULL;
    }
  }

 /*
  * In case we changed it, set Feb. back to 28 days. 
  */

  days[2] = 28;
  free(vector);
  return sd;
}

split_email *splitemail(char *email)
{
  int nvecs, i;
  int *vector;
  struct split_email *se = NULL;
  char *buffer, *sptr;
  static int r_erroffset;
  static const char *r_errptr;
  static pcre *email_code = NULL;


 /*
  * If we haven't been called yet, compile the regexps.
  */
     
  if (!email_code) {
    if (!(email_code = pcre_compile(EMAIL_R,0,&r_errptr,
						&r_erroffset,NULL))) {
      printf("Pattern 1 failed to compile.\n%d: %s\n",r_erroffset,r_errptr);
      return NULL;
    }
  }
      
 /*  
  * Allocate the vector for the regex.
  */
      
  if (!(vector = malloc(sizeof(int) * VECSIZE))) {
    return NULL;
  }
    
 /*   
  * Test the string against the compiled regex.
  */
      
  if ((nvecs = pcre_exec(email_code,NULL,email,strlen(email),
			 0,0,vector,VECSIZE)) >= 0) {
      
   /*
    * Allocate the struct for the split email address.
    */
    
    if (!(se = malloc(sizeof(struct split_email) + 1 + strlen(email)))) {
      free(vector);
      return NULL;
    }

   /*
    * Set a pointer to the space after the email struct and copy our
    * string there.
    */

    buffer = (char *) se + sizeof(struct split_email);
    strcpy(buffer,email);
     
   /*
    * If the regex covers the whole string, set the end pointer to NULL.
    * Otherwise, point it to the character following the matched string.
    */
    
    if (vector[1] == strlen(email)) {
      se->endptr = NULL;
    } else {
      se->endptr = email + vector[1];
    }
   
   /*
    * Populate the structure with pointers to the buffer.
    */

    se->email = buffer + vector[2];
    se->hostname = buffer + vector[4];
    *(buffer + vector[5]) = '\0';
  }
  free(vector);
  return se;
}

split_int *splitint(char *number)
{
  int nvecs, *vector;
  char *buffer;
  struct split_int *si=NULL;
  static int r_erroffset;
  static const char *r_errptr;
  static pcre *int_code = NULL;

 /*
  * If we haven't been called yet, compile the regexp.
  */
     
  if (!int_code) {
    if (!(int_code = pcre_compile(INTEGER_R,0,&r_errptr,
						&r_erroffset,NULL))) {
      printf("Pattern 1 failed to compile.\n%d: %s\n",r_erroffset,r_errptr);
      return NULL;
    }
  }

 /*  
  * Allocate the vector for the regex.
  */

  if (!(vector = malloc(sizeof(int) * VECSIZE))) {
    return NULL;
  }
    
 /*   
  * Test the string against the compiled regex.
  */
      
  if ((nvecs = pcre_exec(int_code,NULL,number,strlen(number),
			 0,0,vector,VECSIZE)) >= 0) {
      
   /*
    * Allocate the struct for the split integer.
    */
      
    if (!(buffer = malloc(1 + vector[1] - vector[0]))) {
      free(vector);
      return NULL;
    }
    if (!(si = malloc(sizeof(struct split_int)))) {
      free(buffer);
      free(vector);
      return NULL;
    }
 
   /*
    * If the regex covers the whole string, set the end pointer to NULL.
    * Otherwise, point it to the character following the matched string.
    */

    if (vector[1] == strlen(number)) {
      si->endptr = NULL;
    } else {
      si->endptr = number + vector[1];
    } 
     
   /*
    * Change the string to a long.
    */

    strncpy(buffer,number + vector[2],1 + vector[1] - vector[0]);
    si->value = strtol(buffer,NULL,10);
    free(buffer);
  }
  free(vector);
  return si;
}

split_name *splitname(char *name)
{
  int nvecs, *vector;
  struct split_name *sn = NULL;
  char *buffer;  
  static int r_erroffset;
  static const char *r_errptr;
  static pcre *lnfnmi_code = NULL,
	      *fnmiln_code = NULL;

 /*
  * If we haven't been called yet, compile the regexps.
  */

  if (!lnfnmi_code) {
    if (!(lnfnmi_code = pcre_compile(LNFNMI_R,0,&r_errptr,&r_erroffset,NULL))) {
      printf("Pattern 1 failed to compile.\n%d: %s\n",r_erroffset,r_errptr);
      return NULL;
    }
    if (!(fnmiln_code = pcre_compile(FNMILN_R,0,&r_errptr,&r_erroffset,NULL))) {
      printf("Pattern 2 failed to compile.\n%d: %s\n",r_erroffset,r_errptr);
      return NULL;
    }
  }

 /*   
  * Allocate the vector for the regex.
  */

  if (!(vector = malloc(sizeof(int) * VECSIZE))) {
    return NULL;
  }

 /*   
  * Test the string against the compiled regex.
  */

  if ((nvecs = pcre_exec(lnfnmi_code,NULL,name,strlen(name),
			 0,0,vector,VECSIZE)) >= 0) {

   /*
    * Allocate the struct for the split name + space for the strings.
    */

    if (!(sn = malloc(sizeof(struct split_name) + 1 + strlen(name)))) {
      free(vector);
      return NULL;
    }

   /*
    * Set a pointer to the space after the name struct and copy our string
    * there.
    */

    buffer = (char *) sn + sizeof(struct split_name);
    strcpy(buffer,name);

   /*
    * If the regex covers the whole string, set the end pointer to NULL.
    * Otherwise, point it to the character following the matched string.
    */                          

    if (vector[1] == strlen(name)) {
      sn->endptr = NULL;
    } else {
      sn->endptr = name + vector[1];
    } 

   /*
    * Populate the structure with pointers to the buffer and split the
    * buffer into the firstname, mi and lastname parts.
    */

    sn->lastname = buffer + vector[2];
    *(buffer + vector[3]) = '\0';
    sn->firstname = buffer + vector[4];
    *(buffer + vector[5]) = '\0';

   /*
    * Do we have a middle initial?
    */

    if (nvecs == 5) {
      sn->mi = buffer + vector[8];
      *(buffer + vector[9]) = '\0';
    } else {
      sn->mi = NULL;
    }

 /*   
  * Test the string against the compiled regex.
  */

  } else if ((nvecs = pcre_exec(fnmiln_code,NULL,name,strlen(name),
				0,0,vector,VECSIZE)) >= 0) {

   /*
    * Allocate the struct for the split name + space for the strings.
    */

    if (!(sn = malloc(sizeof(struct split_name) + 1 + strlen(name)))) {
      free(vector);
      return NULL;
    }

   /*
    * Set a pointer to the space after the name struct and copy our string
    * there.
    */

    buffer = (char *) sn + sizeof(struct split_name);
    strcpy(buffer,name);

   /*
    * If the regex covers the whole string, set the end pointer to NULL.
    * Otherwise, point it to the character following the matched string.
    */                          

    if (vector[1] == strlen(name)) {
      sn->endptr = NULL;
    } else {
      sn->endptr = name + vector[1];
    } 

   /*
    * Populate the structure with pointers to the buffer and split the
    * buffer into the firstname, mi and lastname parts.
    */

    sn->firstname = buffer + vector[2];
    *(buffer + vector[3]) = '\0';

   /*
    * Do we have a middle initial?
    */

    if (vector[6] != -1) {
      sn->mi = buffer + vector[6];
      *(buffer + vector[7]) = '\0';
    } else {
      sn->mi = NULL;
    }
    sn->lastname = buffer + vector[8];
    *(buffer + vector[9]) = '\0';
  }
  free(vector);
  return sn;
}

split_telephone *splittelephone(char *tel)
{
  int nvecs, i;
  int *vector;
  struct split_telephone *st = NULL;
  char *buffer, *sptr;
  static int r_erroffset;
  static const char *r_errptr;
  static pcre *tel_code1 = NULL,
	      *tel_code2 = NULL;


 /*
  * If we haven't been called yet, compile the regexps.
  */
     
  if (!tel_code1) {
    if (!(tel_code1 = pcre_compile(TELEPHONE1_R,0,&r_errptr,
						&r_erroffset,NULL))) {
      printf("Pattern 1 failed to compile.\n%d: %s\n",r_erroffset,r_errptr);
      return NULL;
    }
    if (!(tel_code2 = pcre_compile(TELEPHONE2_R,0,&r_errptr,
						&r_erroffset,NULL))) {
      printf("Pattern 1 failed to compile.\n%d: %s\n",r_erroffset,r_errptr);
      return NULL;
    }
  }
      
 /*  
  * Allocate the vector for the regex.
  */
      
  if (!(vector = malloc(sizeof(int) * VECSIZE))) {
    return NULL;
  }
    
 /*   
  * Test the string against the compiled regex.
  */
      
  if ((nvecs = pcre_exec(tel_code1,NULL,tel,strlen(tel),
			 0,0,vector,VECSIZE)) >= 0) {
      
   /*
    * Allocate the struct for the split telephone.
    */
    
    if (!(st = malloc(sizeof(struct split_telephone) + 1 + strlen(tel)))) {
      free(vector);
      return NULL;
    }

   /*
    * Set a pointer to the space after the telephone struct and copy our
    * string there.
    */

    buffer = (char *) st + sizeof(struct split_telephone);
    strcpy(buffer,tel);
     
   /*
    * If the regex covers the whole string, set the end pointer to NULL.
    * Otherwise, point it to the character following the matched string.
    */
    
    if (vector[1] == strlen(tel)) {
      st->endptr = NULL;
    } else {
      st->endptr = tel + vector[1];
    }
   
   /*
    * Populate the structure with pointers to the buffer and split the
    * buffer into the area code and number parts.
    */

    if (vector[4] != -1) {
      st->areacode = buffer + vector[4];
      *(buffer + vector[5]) = '\0';
    } else if (vector[6] != -1) {
      st->areacode = buffer + vector[6];
      *(buffer + vector[7]) = '\0';
    } else {
      st->areacode = NULL;
    }
    st->number = buffer + vector[8];
    *(buffer + vector[9]) = '\0';
    
 /*   
  * Test the string against the compiled regex.
  */
      
  } else if ((nvecs = pcre_exec(tel_code2,NULL,tel,strlen(tel),
			 0,0,vector,VECSIZE)) >= 0) {
      
   /*
    * Allocate the struct for the split telephone.
    */
    
    if (!(st = malloc(sizeof(struct split_telephone) + 3 + strlen(tel)))) {
      free(vector);
      return NULL;
    }

   /*
    * Set up pointers to our new buffer and the found number in our test
    * string.
    */

    buffer = (char *) st + sizeof(struct split_telephone);
    sptr = tel + vector[0];
     
   /*
    * If the regex covers the whole string, set the end pointer to NULL.
    * Otherwise, point it to the character following the matched string.
    */
    
    if (vector[1] == strlen(tel)) {
      st->endptr = NULL;
    } else {
      st->endptr = tel + vector[1];
    }
   
   /*
    * Populate the structure with pointers to the buffer and split the
    * buffer into the area code and number parts. We have to copy the 
    * strings 1 byte at a time so that we can insert the nulls and the
    * dash at the proper places.
    */

    if (vector[2] != -1) {
      st->areacode = buffer;
      for (i=0; i<3; i++)
        *buffer++ = *sptr++;
      *buffer++ = '\0';
    } else {
      st->areacode = NULL;
    }
    st->number = buffer;
    for (i=0; i<3; i++)
      *buffer++ = *sptr++;
    *buffer++ = '-';
    for (i=0; i<4; i++)
      *buffer++ = *sptr++;
    *buffer = '\0';
  }
  free(vector);
  return st;
}

split_time *splittime(char *tt)
{
  int nvecs, *vector;
  char *buffer;
  struct split_time *st=NULL;
  static int r_erroffset;
  static const char *r_errptr;
  static pcre *time_code = NULL;

    
 /*
  * If we haven't been called yet, compile the regexp.
  */
     
  if (!time_code) {
    if (!(time_code = pcre_compile(TIME_R,0,&r_errptr,&r_erroffset,NULL))) {
      printf("Pattern 1 failed to compile.\n%d: %s\n",r_erroffset,r_errptr);
      return NULL;
    }
  }

 /*  
  * Allocate the vector for the regex.
  */

  if (!(vector = malloc(sizeof(int) * VECSIZE))) {
    return NULL;
  }
    
 /*   
  * Test the string against the compiled regex.
  */
      
  if ((nvecs = pcre_exec(time_code,NULL,tt,strlen(tt),
			 0,0,vector,VECSIZE)) >= 0) {
      
   /*
    * Allocate the struct for the split time.
    */
      
    if (!(st = malloc(sizeof(struct split_time)))) {
      free(vector);
      return NULL;
    }
 
   /*
    * If the regex covers the whole string, set the end pointer to NULL.
    * Otherwise, point it to the character following the matched string.
    */

    if (vector[1] == strlen(tt)) {
      st->endptr = NULL;
    } else {
      st->endptr = tt + vector[1];
    } 
     
   /*
    * Compute the hour, minute, (second, (millisecond)).
    */

    st->hour = strtol(tt + vector[2],NULL,10);
    st->minute = strtol(tt + vector[4],NULL,10);
    if (nvecs >= 5) {
      st->second = strtol(tt + vector[8],NULL,10);
      if (nvecs >= 6) {
        st->millisecond = strtol(tt + vector[12],NULL,10);
      } else {
        st->millisecond = 0;
      }
    } else {
      st->second = 0;
    }
  }
  free(vector);
  return st;
}

split_zipcode *splitzipcode(char *zip)
{
  int nvecs;
  int *vector;
  struct split_zipcode *sz = NULL;
  char *buffer;
  static int r_erroffset;
  static const char *r_errptr;
  static pcre *zip_code = NULL;


 /*
  * If we haven't been called yet, compile the regexp.
  */
     
  if (!zip_code) {
    if (!(zip_code = pcre_compile(ZIPCODE_R,0,&r_errptr,
						&r_erroffset,NULL))) {
      printf("Pattern 1 failed to compile.\n%d: %s\n",r_erroffset,r_errptr);
      return NULL;
    }
  }

 /*
  * Allocate the vector for the regex.
  */

  if (!(vector = malloc(sizeof(int) * VECSIZE))) {
    return NULL;
  }
     
 /*
  * Test the string against the compiled regex.
  */

  if ((nvecs = pcre_exec(zip_code,NULL,zip,strlen(zip),
			 0,0,vector,VECSIZE)) >= 0) {

   /*
    * Allocate the struct for the split zipcode + space for the strings.
    */

    if (!(sz = malloc(sizeof(struct split_zipcode) + 2 + strlen(zip)))) {
      free(vector);
      return NULL;
    }

   /*
    * Set a pointer to the space after the zipcode struct and copy our string
    * there.
    */
     
    buffer = (char *) sz + sizeof(struct split_zipcode);
    strcpy(buffer,zip);
     
   /*
    * If the regex covers the whole string, set the end pointer to NULL.
    * Otherwise, point it to the character following the matched string.
    */
    
    if (vector[1] == strlen(zip)) {
      sz->endptr = NULL;
    } else {
      sz->endptr = zip + vector[1];
    }
   
   /*
    * Populate the structure with pointers to the buffer and split the
    * buffer into the zip and plus4 parts.
    */

    sz->zip = buffer + vector[2];
    *(buffer + vector[3]) = '\0';
    if (nvecs > 2) {
      sz->plus4 = buffer + vector[6];
      *(buffer + vector[7]) = '\0';
    } else {
      sz->plus4 = NULL;
    }
  }
  free(vector);
  return sz;
}
