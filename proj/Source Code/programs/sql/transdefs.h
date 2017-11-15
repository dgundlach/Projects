// Reserve subaccount.  Goes out to cash.

#define MOVE_PAYMENT_TO_RESERVE		100     // In
#define TRANS_PAYMENT_FROM_RESERVE	150     // Out

// Prepayment subaccount.  Goes out to reserve.

#define MOVE_PAYMENT_TO_PREPAID		200     // In
#define MOVE_PREPAID_MONTH_TO_RESERVE	250     // Out

// Credits subaccount.  Goes out to credit.

#define TRANS_CREDIT_EXTERNAL           300     // In
#define TRANS_PREPAYMENT_ADJUSTMENT	301     // In
#define TRANS_PAYMENT_FROM_CREDIT	350     // Out

// Free months subaccount.  Goes out to credits.

#define TRANS_GRANT_FREE_MONTHS		400     // In
#define MOVE_FREE_MONTH_TO_CREDITS	450     // Out

// Cash subaccount.  Goes out to reserve.

#define TRANS_CASH_FUNDS		500     // In
#define TRANS_CREDIT_CARD_FUNDS		501     // In
#define TRANS_DEBIT_CARD_FUNDS		502     // In
#define TRANS_CHECK_FUNDS		503     // In
#define TRANS_AUTO_CHECK_FUNDS		504     // In

// Cash suubaccount.  Goes out to prepayment.

#define TRANS_SETUP_PREPAID_CASH	505     // In
#define TRANS_SETUP_PREPAID_CREDIT_CARD	506     // In
#define TRANS_SETUP_PREPAID_DEBIT_CARD	507     // In
#define TRANS_SETUP_PREPAID_CHECK	508     // In

// Cash subaccount.  This is where we get paid.

#define TRANS_MONTHLY_FEE		550     // Out
#define TRANS_SETUP_FEE			551     // Out
#define TRANS_PENALTY_FEE		552     // Out
#define TRANS_MISC_FEE			553     // Out

// Cash subaccount.  Cancels payments and everything in their wake.

#define TRANS_REVOKE_FUNDS		599     // Destroy

// Referrals subaccount.  Goes out to credits.

#define TRANS_REFERRAL_ENABLED		600     // In
#define MOVE_REFERRAL_MONTH_TO_CREDITS	650     // Out

// Credits subaccount.  This one comes in from the outside and gets 
// redirected to TRANS_CREDIT_EXTERNAL.

#define TRANS_CREDIT_EXTERNAL_IN	700     // In

// Miscellaneous transactions affecting status.

#define TRANS_ADDED_ACCOUNT		900
#define TRANS_LOCKED_ACCOUNT		901
#define TRANS_UNLOCKED_ACCOUNT		902
#define TRANS_SUSPENDED_ACCOUNT		903
#define TRANS_AUTO_SUSPENDED_ACCOUNT	904
#define TRANS_UNSUSPENDED_ACCOUNT	905
#define TRANS_REMOVED_ACCOUNT		999

#define FUNDS_IN_MIN			500
#define FUNDS_IN_MAX			549
#define FEE_TYPE_MIN			550
#define FEE_TYPE_MAX                    598
#define MIN_TAKE_ACTION			500

#define TRANS_PREPAY_FOR_FREE_MONTH	6

#define PRORATE				7
