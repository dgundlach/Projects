create or replace function
          reconcile_amounts(integer, integer, billing, integer)
          returns integer as '
declare
    tacctno alias for $1;
    ttype alias for $2;
    user alias for $3;
    transno alias for $4;
    money float8;
    amt float8;
    res float8;
    cred float8;
    t_rec record;
    total_pd float8;
BEGIN
    money := user.reserve + user.credit;
    amt := 0;
-- Find all unreconciled fees and process them if possible.
    for t_rec in select *
                   from transactions
                  where acctno = tacctno and r_seq = -1
                        and type >= 550
                        and type <= 598
               order by t_seq loop
        if t_rec.amount + amt <= money then
            amt := amt + t_rec.amount;
            update transactions set r_seq = transno
                              where acctno = tacctno
                                and t_seq = t_rec.t_seq;
-- Mark the setup fee as paid.
            if t_rec.type = 551 then
                user.setup_paid := ''Y'';
-- Add 1 to the expire interval.
            elsif t_rec.type = 550 then
                user.expire_interval := user.expire_interval + 1;
            end if;
        end if;
    end loop;
-- Subtract the sum of all reconciled amounts from the balance.
    user.balance := user.balance - amt;
-- Figure out which pile the money is coming from, starting with the
-- reserve.
    if user.reserve >= amt then
        res := amt;
    else
        res := user.reserve;
    end if;
    cred := amt - res;
-- Update the reserve and credit fields.
    user.reserve = user.reserve - res;
    user.credit = user.credit - cred;
    user.total_paid := user.total_paid + res;
-- Insert transaction(s) for the amount(s) taken from the reserve and
-- credits.
    if res > 0 then
        user.t_seq := user.t_seq + 1;
        insert into transactions
                    (acctno, t_seq, entered_by, type, amount, r_seq)
             values (tacctno, user.t_seq, current_user,
                     150, res, transno);
    end if;
    if cred > 0 then
        user.t_seq := user.t_seq + 1;
        insert into transactions
                    (acctno, t_seq, entered_by, type, amount, r_seq)
             values (tacctno, user.t_seq, current_user,
                     350, cred, transno);
    end if;
-- If the customer has been referred by another, see if it is time to
-- apply the referral credit.
    if user.referred_by != -1 then
        select into total_pd total_paid from referrals where
                  acctno = user.referred_by;
        if total_pd <= user.total_paid then
            user.t_seq := user.t_seq + 1;
            insert into transactions
                        (acctno, t_seq, entered_by, type, amount,
                         r_seq, comment)
                 values (tacctno, user.t_seq, current_user,
                         600, 0, transno,
                         quote_literal(t_rec.acctno));
            update billing set referrals = referrals + 1,
                               referral_credits = referral_credits + 1
                         where acctno = user.referred_by;
-- Mark the referral field so that we do not perpetually give the referring
-- customer a credit.
            user.referred_by := -1;
        end if;
    end if;
-- Update all the fields which may have been changed in this routine
-- or any routine that calls this one.
    update billing set expire_interval = user.expire_interval,
                       billing_interval = user.billing_interval,
                       ppd_months = user.ppd_months,
                       ppd_ppm = user.ppd_ppm,
                       ppd_seq = user.ppd_seq,
                       credit_months = user.credit_months,
                       ppd_months_o = user.ppd_months_o,
                       ppd_ppm_o = user.ppd_ppm_o,
                       ppd_seq_o = user.ppd_seq_o,
                       credit_months_o = user.credit_months_o,
                       referral_months = user.referral_months,
                       suspended = quote_literal(user.suspended),
                       referred_by = user.referred_by,
                       setup_fee_paid = user.setup_fee_paid,
                       total_paid = user.total_paid,
                       credit = user.credit,
                       reserve = user.reserve,
                       balance = user.balance,
                       t_seq = t_seq + 1
                 where acctno = user.acctno;
    return ttype;
end;
' language 'plpgsql';
create or replace function
          apply_fee(integer, integer, float8, billing, integer)
          returns integer as '
declare
    tacctno alias for $1;
    ttype alias for $2;
    tamount alias for $3;
    user alias for $4;
    transno alias for $5;
    ntype int;
    ntransno int;
    namount float8;
    amt float8;
    mon int;
begin
    ntype := 0;
    namount := 0;
    ntransno := transno;
    mon := 1;
    user.balance := user.balance + tamount;
-- Setup fee. Mark the setup fee as unpaid.
    if ttype = 551 then
        user.setup_fee_paid := ''N'';
    end if;
-- If the fee is not the monthly fee, reconcile the amounts and exit.
    if ttype != 550 then
        return reconcile_amounts(tacctno, ttype, user, transno);
    end if;
-- If the customer has any pending referral credits, apply one of them.
    if user.referral_credits > 0 then
        ntype := 650;
        namount := tamount;
        user.credit := user.credit + namount;
        user.referral_months := user.referral_months - 1;
-- If the customer has prepaid, apply the payment.
    elsif user.ppd_months > 0 then
        ntype := 250;
        namount := user.ppd_ppm;
        user.reserve := user.reserve + namount;
        ntransno := user.ppd_seq;
-- If this is the last month in the main block of prepaid months, move
-- the overflow into place if this block has no associated credits.
        if user.ppd_months = 1 then
            if user.credit_months = 0 and (user.ppd_months_o > 0
                                        or user.credit_months > 0) then
                user.ppd_months := user.ppd_months_o;
                user.ppd_ppm := user.ppd_ppm_o;
                user.ppd_seq := user.ppd_seq_o;
                user.credit_months := user.credit_months_o;
                user.ppd_months_o := 0;
                user.ppd_ppm_o := user.monthly_fee;
                user.ppd_seq_o := -1;
                user.credit_months_o := 0;
-- There is nothing to move. Just set the number of prepaid months to 0.
            else
                user.ppd_ppm := user.monthly_fee;
                user.ppd_months := 0;
            end if;
-- There are more months left. Just subtract 1.
        else
            user.ppd_months := user.ppd_months - 1;
        end if;
-- If the customer has any credit months, apply one.
    elsif user.credit_months > 0 then
        ntype := 450;
        namount := tamount;
        user.credit := user.credit + namount;
-- If we can tie this credit month to a particular prepaid transaction,
-- do so.
        if user.ppd_seq != -1 then
            ntransno := user.ppd_seq;
        end if;
-- If this is the last month in the block of credit months, move the
-- overflow into place if there is one.
        if user.credit_months = 1 then
            if user.ppd_months_o > 0 or user.credit_months > 0 then
                user.ppd_months := user.ppd_months_o;
                user.ppd_ppm := user.ppd_ppm_o;
                user.ppd_seq := user.ppd_seq_o;
                user.credit_months := user.credit_months_o;
                user.ppd_months_o := 0;
                user.ppd_ppm_o := user.monthly_fee;
                user.ppd_seq_o := -1;
                user.credit_months_o := 0;
            else
                user.ppd_ppm := user.monthly_fee;
            end if;
-- There are more months left. Just subtract 1.
        else
            user.credit_months := user.credit_months - 1;
        end if;
-- Check if the account is in arrears, and suspend it if that is the case.
    else
        amt := user.balance - user.credit - user.reserve;
        if user.billing_interval > 0 and amt > user.monthly_fee then
            user.t_seq := user.t_seq + 1;
            insert into transactions
                       (acctno, t_seq, entered_by, type, amount, r_seq)
                values (tacctno, user.t_seq, current_user,
                        904, 0, ntransno);
            user.suspended := ''A'';
        end if;
    end if;
-- Generate the transaction that was built above.
    if namount != 0 then
        user.t_seq := user.t_seq + 1;
        insert into transactions
                   (acctno, t_seq, entered_by, type, amount, r_seq, months)
            values (tacctno, user.t_seq, current_user,
                    ntype, namount, ntransno, mon);
    end if;
-- Increment the billing interval and reconcile the account.
    user.billing_interval := user.billing_interval + 1;
    return reconcile_amounts(tacctno, ttype, user, ntransno);
end;
' language 'plpgsql';
create or replace function
          setup_prepaid(integer, integer, float8, integer,
          billing, integer) returns integer as '
declare
    tacctno alias for $1;
    ttype alias for $2;
    tamount alias for $3;
    tmonths alias for $4;
    user alias for $5;
    transno alias for $6;
    cnt int;
    trans_sum float8;
    ppm float8;
    diff float8;
    overpay float8;
    free int;
begin
-- Grab a quick count of the number of months that the customer owes for,
-- along with the total price they represent.
    select into cnt, trans_sum count(*), sum(amount) from transactions
          where acctno = tacctno and r_seq = -1
            and type = 550;
-- If there are insufficient funds to create a block of prepaid months,
-- or if the number of months we are trying to set up minus the number
-- already owed is less than 3, just add the amount to the customer''s
-- reserve.
    if months - cnt <= 3
            or (amount + user.reserve) <= user.balance then
        raise notice "Changing the transaction type to a regular payment.  It makes";
        raise notice "no sense to set up a block of months with this amount.";
        ttype := ttype - 5;
        user.reserve := user.reserve + amount;
        return reconcile_amounts(tacctno, ttype, user, transno);
    end if;
-- Calculate the number of free months that this transaction will grant,
-- and generate a transaction for them.
    free := months / 6;
    if free > 0 then
        user.t_seq := user.t_seq + 1;
        insert into transactions
                   (acctno, t_seq, entered_by, type, amount, r_seq, months)
            values (tacctno, user.t_seq, current_user,
                    GRANT_FREE_MONTHS, 0, transno, free);
    end if;
-- This is the amount that goes to the reserve to pay other fees.
    overpay := user.balance - trans_sum;
-- Calculate the price per month
    ppm := (amount - overpay) / months;
-- Calculate the difference in the price per month charged and the computed
-- price per month multiplied by the number of months outstanding. If the
-- value is positive, the calculated price per month is less than the
-- customer''s normal price per month. A credit needs to be applied to
-- bring the outstanding transactions in line with the new price per month.
    diff := trans_sum - (ppm * cnt);
    if diff > 0 then
        user.credit := user.credit + diff;
        user.t_seq := user.t_seq + 1;
        insert into transactions
                   (acctno, t_seq, entered_by, type, amount, r_seq)
            values (tacctno, user.t_seq, current_user,
                    301, diff, transno);
-- If the calculated price per month is greater than the customer''s normal
-- price per month, move the difference to the customer''s reserve.
    else
        overpay := overpay - diff;
    end if;
    user.reserve := user.reserve + overpay;
-- Be sure to put what is happening to the reserve into the audit trail.
    if overpay != 0 then
        user.t_seq := user.t_seq + 1;
        insert into transactions
                   (acctno, t_seq, entered_by, type, amount, r_seq)
            values (tacctno, user.t_seq, current_user,
                    100, overpay, transno);
    end if;
-- Insert a transaction for the transfer of the funds to the prepaid
-- block.
    user.t_seq := user.t_seq + 1;
    insert into transactions
               (acctno, t_seq, entered_by, type, amount, r_seq, months)
        values (tacctno, user.t_seq, current_user,
                200, ppm * months, transno,
                months);
-- Insert a transaction for the month(s) outstanding. Add the amounts
-- for the transactions to the reserve.
    if cnt > 0 then
        user.t_seq := user.t_seq + 1;
        insert into transactions
                   (acctno, t_seq, entered_by, type, amount, r_seq)
            values (tacctno, user.t_seq, current_user,
                    250, ppm * cnt, transno);
    end if;
    user.reserve := user.reserve + (ppm * cnt);
-- Calculate the new months count.
    cnt := months - cnt;
-- If the overflow is not filled, then we''re ok to go.
    if user.ppd_months_o = 0 then
-- Check the first block.
        if user.ppd_months = 0 and user.credit_months = 0 then
            user.ppd_months := cnt;
            user.ppd_ppm := ppm;
            user.ppd_seq := transno;
            user.credit_months := free;
-- The first block is filled. Put it in the second block.
        else
            user.ppd_months_o := cnt;
            user.ppd_ppm_o := ppm;
            user.ppd_seq_o := transno;
            user.credit_months_o := free;
    else
        raise exception ''There are already 2 blocks of prepaid months.'';
    end if;
    return reconcile_amounts(tacctno, ttype, user, transno);
end;
' language 'plpgsql';
create or replace function
          revoke_funds(integer, float8, billing, integer)
          returns integer as '
declare
    tacctno alias for $1;
    tamount alias for $2;
    user alias for $3;
    transno alias for $4;
    t_rec record;
    parent int;
    parent_t int;
    amt float8;
    bal float8;
    cred float8;
    res float8;
    mon int;
    cm int;
    pm int;
    block int;
    usecm bool;
    usepm; bool;
begin
    cred := 0;
    res := 0;
    mon := 0;
    cm := 0;
    pm := 0;
    block := 0;
    usecm := false;
    usepm := false;
    select into t_rec *
           from transactions
          where acctno = tacctno and amount = tamount
                                      and type >= 500
                                      and type <= 549
                                      and revoked = ''N''
       order by t_seq desc
          limit 1;
    parent := t_rec.t_seq;
    parent_t := t_rec.type;
    amt := t_rec.amount;
    bal := t_rec.amount;
    for t_rec in select *
                   from transactions
                  where acctno = tacctno and (r_seq = parent
                                                or t_seq = parent)
               order by t_seq desc loop
        if t_rec.type < 550 then
            update transactions set revoked = ''Y'',
                                    r_seq = t_seq
                              where acctno = tacctno
                                and t_seq = t_rec.t_seq;
        elsif t_rec.type >= 550
                and t_rec.type <= 598 then
            update transactions set r_seq = -1
                              where acctno = tacctno
                                and t_seq = t_rec.t_seq;
        end if;
        if t_rec.type = 100 then
            res := res - t_rec.amount;
        elsif t_rec.type = 150 then
            res := res + t_rec.amount;
        elsif t_rec.type = 200 then
            pm := pm - t_rec.months;
            usepm := true;
        elsif t_rec.type = 250 then
            pm := pm + t_rec.months;
            usepm := true;
        elsif t_rec.type = 300 then
            cred := cred - t_rec.amount;
        elsif t_rec.type = 301 then
            cred := cred - t_rec.amount;
        elsif t_rec.type = 350 then
            cred := cred + t_rec.amount;
        elsif t_rec.type = 400 then
            cm := cm - t_rec.months;
            usecm := true;
        elsif t_rec.type = 450 then
            cm := cm + t_rec.months;
            usecm := true;
        elsif t_rec.type >= 500
                and t_rec.type < 550 then
            bal := bal - t_rec.amount;
        elsif t_rec.type = 550 then
            bal := bal + t_rec.amount;
            mon := mon + 1;
        elsif t_rec.type = 551 then
            bal := bal + t_rec.amount;
            user.setup_fee_paid := ''N'';
        elsif t_rec.type = 552
                or t_rec.type = 553 then
            bal := bal + t_rec.amount;
        end if;
    end loop;
    user.balance := user.balance + bal;
    user.credit := user.credit + cred;
    user.reserve := user.reserve + res;
    if parent_t >= 505
            and parent_t <= 508 then
        if user.ppm_seq = parent then
            pm := pm + user.ppd_months;
            cm := cm + user.credit_months;
            user.ppd_months := user.ppd_months_o;
            user.credit_months := user.credit_months_o;
            user.ppd_ppm := user.monthly_fee_o;
            user.ppd_months_o := 0;
            user.credit_months_o := 0;
            user.ppd_ppm_o := user.monthly_fee;
        elsif user.ppm_seq_o = parent then
            pm := pm + user.ppd_months_o;
            cm := cm + user.credit_months_o;
            user.ppd_months_o := 0;
            user.credit_months_o := 0;
            user.ppd_ppm_o := user.monthly_fee;
        end if;
        if pm != 0 or cm != 0 then
            raise exception "Database is out of sync.";
        end if;
    end if;
    return reconcile_amounts(tacctno, 599, user, transno);
end;
' language 'plpgsql';
create or replace function preprocess_trans() returns trigger as '
declare
    rename new to trans;
    transno int;
    user billing%ROWTYPE;
    i int;
begin
    transno = trans.t_seq;
-- Only fetch the user record if we absolutely have to.
    if trans.type >= 500 and trans.type < 900 then
        select into user * from billing where acctno = trans.acctno;
    end if;
    if trans.type = 550 then
-- If the account is suspended, do not bill. Just increment the billing
-- and expire intervals, then throw away the transaction.
        if user.suspended != ''N'' then
            update billing set t_seq = t_seq + 1,
                               billing_interval = billing_interval + 1,
                               expire_interval = expire_interval + 1
                         where acctno = trans.acctno;
            return NULL;
        else
-- If the customer has prepaid and has no referral credits, adjust
-- the monthly fee to reflect the price per month that the customer
-- has prepaid. Only do so if the amount prepaid is less than the
-- current monthly fee.
            if user.referral_credits = 0 and user.ppd_months > 0 then
                transno = user.ppd_seq;
                if trans.amount > user.ppd_ppm then
                    trans.amount := user.ppd_ppm;
                end if;
            end if;
            i := apply_fee(trans.acctno, trans.type, trans.amount,
                           user, transno);
        end if;
-- Setup fee and other fees.
    elsif trans.type in (551,
                         552,
                         553) then
        i := apply_fee(trans.acctno, trans.type, trans.amount, user, transno);
-- Payment from cash, check, credit card, etc.
    elsif trans.type in (500,
                         501,
                         502,
                         503,
                         504) then
        user.t_seq := user.t_seq + 1;
        insert into transactions
                   (acctno, t_seq, entered_by, type, amount, r_seq)
            values (trans.acctno, user.t_seq, current_user,
                    100, overpay, transno);
        user.reserve := user.reserve + trans.amount;
        i := reconcile_amounts(trans.acctno, trans.type, user, transno);
-- Revoke funds entered by mistake or for insufficient funds.
    elsif trans.type = 599 then
        i := revoke_funds(trans.acctno, trans.amount, user, transno);
-- Credit for a partial month. Usually used when setting up a customer
-- in the middle of the month to prorate his first month.
    elsif trans.type = 700 then
        user.credit := user.credit + trans.amount;
        trans.type := 300;
        i := reconcile_amounts(trans.acctno, trans.type, user, transno);
-- Add the account or "unremove" it.
    elsif trans.type = 900 then
        update billing set t_seq = t_seq + 1,
                           removed = ''N''
                     where acctno = trans.acctno;
-- "Remove" the account. Does not really do much of anything except
-- hide the account from queries in the user level programs.
    elsif trans.type = 999 then
        update billing set t_set = t_seq + 1,
                           removed = ''Y''
                     where acctno = trans.acctno;
-- Lock the account.
    elsif trans.type = 901 then
        update billing set t_seq = t_seq + 1,
                           locked = ''Y''
                     where acctno = trans.acctno;
-- Unlock the account.
    elsif trans.type = 902 then
        update billing set t_seq = t_seq + 1,
                           locked = ''N''
                     where acctno = trans.acctno;
-- Suspend the account. This one is used manually. There is another
-- type of suspension used by these routines.
    elsif mew.type = 903 then
        update billing set t_seq = t_seq + 1,
                           suspended = ''Y''
                     where acctno = mew.acctno;
-- Unsuspend the account. This works for both types of suspension.
    elsif trans.type = 905 then
        update billing set t_seq = t_seq + 1,
                           suspended = ''N''
                     where acctno = trans.acctno;
-- Set up a block of prepaid months.
    elsif trans.type in (505,
                         506,
                         507,
                         508) then
        i := setup_prepaid(trans.acctno, trans.type, trans.amount,
                           trans.months, user, transno);
        if i != trans.type then
            trans.type := i;
            trans.months := 1;
        end if;
-- On all other transactions, just increment the transaction sequence
-- number.
    else
        update billing set t_seq = t_seq + 1
                     where acctno = trans.acctno;
    end if;
    return trans;
end;
' language 'plpgsql';
drop trigger update_accounting on transactions;
create trigger update_accounting before insert on transactions
    for each row execute procedure preprocess_trans();
