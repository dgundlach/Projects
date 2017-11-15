




struct billing {
  int32    billing_interval;
  int32    credit_months;
  int32    credit_months_o;
  int32    expire_interval;
  int32    ppd_months;
  int32    ppd_months_o;
  int32    ppd_seq;
  int32    ppd_seq_o;
  int32    referral_months;
  int32    referred_by;
  int32    t_seq;
  float8   balance;
  float8   credit;
  float8   ppd_ppm;
  float8   ppd_ppm_o;
  float8   reserve;
  float8   total_paid;
  char     setup_fee_paid;
  char     suspended;
};

typedef struct billing billing;

struct triginfo {
  TriggerData *trigdata;
  
