#include <stdarg.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#include <isc/result.h>
#include <dhcpctl/dhcpctl.h>

int main (int argc, char **argv) {

     /* All modifications of handles and all accesses of
        handle data happen via dhcpctl_data_string objects. */

     dhcpctl_data_string ipaddrstring = NULL;
     dhcpctl_data_string value = NULL;

     dhcpctl_handle connection = NULL;
     dhcpctl_handle lease = NULL;
     isc_result_t waitstatus;
     struct in_addr convaddr;
     time_t thetime;

     dhcpctl_handle authenticator = NULL;
     const char *keyname = "omapi_key";
     const char *algorithm = "hmac-md5";
     const char *secret = "khY4Ihzby0dvbmNNWTB05GpG7/ynyMNMRVuvu1U3NUmawFydyplRgT464bnvBvQc2KJ+oH7b3LAJUJXj+Fm/KQ==";

     /* Required first step. */

     dhcpctl_initialize ();

     /* Set up the connection to the server. The server
        normally listens on port 7911 unless configured to do
        otherwise. */

     dhcpctl_new_authenticator (&authenticator, keyname,
                           algorithm, secret,
                           strlen(secret) + 1);
     dhcpctl_connect (&connection, "127.0.0.1", 7911, 0);

     /* Create a handle to a lease. This call just
        sets up  local  data structure.  The  server  hasn't
        yet  made  any association between the client's data
        structure and any lease it has. */

     dhcpctl_new_object (&lease, connection, "host");

     /* Create a new data string to store in the handle. */

     memset (&ipaddrstring, 0, sizeof ipaddrstring);

     inet_pton(AF_INET, "10.0.1.111", &convaddr);

     omapi_data_string_new (&ipaddrstring, 4, MDL);

     /* Set the ip-address attribute of the lease handle
        to the given address.  We haven't set any
        other attributes, so when the server makes the
        association, the IP address will be all it uses to
        look up the lease in its tables. */

     memcpy(ipaddrstring->value, &convaddr.s_addr, 4);

     dhcpctl_set_value (lease, ipaddrstring, "ip-address");


     /* Prime the connection with the request to look up
        the lease in the server and fill up the local handle
        with the attributes the  server will send over in
        its answer. */

     dhcpctl_open_object (lease, connection, 0);

     /* Send the message (to look up the lease and send back
        the attribute values  in  the  answer) to the server.
        The value in the variable waitstatus when the function
        returns will be the result from the server. If the
        message couldn't be processed properly by the server,
        then the error will be reflected here. */

     dhcpctl_wait_for_completion (lease, &waitstatus);

     if (waitstatus != ISC_R_SUCCESS) {
        /* server not authoritative */
        exit (0);
     }

     /* Clean up memory we no longer need. */

     dhcpctl_data_string_dereference( &ipaddrstring, MDL);

     /* Get the attribute named "ends" from the lease handle.
        This is a 4-byte integer of the time (in Unix epoch
        seconds) that the lease  will expire. */

     dhcpctl_get_value (&value, lease, "ends");

     memcpy(&thetime, value->value, value->len);
     dhcpctl_data_string_dereference(&value, MDL);

     fprintf (stdout, "ending time is %s",
     ctime(&thetime));
}

