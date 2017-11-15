# 1 "bt.cpp"
-- #include "transdefs.h"

create or replace function initialize_user()
                                       returns trigger as '
declare
rename new to billing;

today date;
da int;
period int;
user users%ROWTYPE;
id varchar;
host varchar;
encrypted varchar;
userid int;
PRORATE int DEFAULT 7;

begin
    billing.added_by := current_user;
    billing.billing_interval := 0;
    billing.expire_interval := 1;
    billing.credit := 0;
    billing.start_date := now();
    if billing.billing_start = ''0001-01-01'' then
        period := 1;
        billing.billing_start := billing.start_date;
    elsif billing.billing_start = ''0001-01-15'' then
        period := 15;
        billing.billing_start := billing.start_date;
        billing.billing_start := billing.billing_start - 15;
    else
        billing.billing_start := billing.start_date;
        return billing;
    end if;
    da := date_part(''day'', billing.start_date);
    if da <= (30 - PRORATE) then
        if da > PRORATE then
            billing.credit := (da - 1) *
                (floor((billing.monthly_fee * 100.0) / 30) / 100.0);
        end if;
    else
        billing.billing_start := billing.billing_start + interval ''1 month'';
    end if;
    billing.billing_start := billing.billing_start - da + period;
    if billing.password = ''!!'' then
        encrypted := billing.password;
    else
        encrypted := crypt(billing.password);
    end if;
    billing.gid := nextval(''gid_seq'');
    id := split_part(billing.email, ''@'', 1);
    host := split_part(billing.email, ''@'', 2);
    insert into users (login,
                       crypt,
                       start_date,
                       expire_date,
                       attr_template,
                       domain,
                       uid,
                       gid,
                       home)
               values (id,
                       encrypted,
                       billing.start_date,
                       billing.billing_start
                           + interval ''1 month'' * billing.expire_interval,
                       ''dynamicppp'',
                       host,
                       billing.gid,
                       billing.gid,
                       host || ''/'' || id);
    return billing;
end;
' language 'plpgsql';

drop trigger insert_billing on billing;
create trigger insert_billing before insert on billing
    for each row execute procedure initialize_user();

create or replace function crypt(varchar) returns varchar as '

my ($password) = @_;
my $b64 = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz"
                  . "0123456789./";
return crypt($password, substr($b64, int(rand(64)), 1)
                      . substr($b64, int(rand(64)), 1));

' language 'plperl';

create or replace function change_user() returns trigger as '
declare
    query text;
    sep text;
    crypted varchar;
    id text;
    host text;
    actv char;
begin
    sep := '' '';
    query := ''update users set'';
    if new.password != old.password then
        if new.password = ''!!'' then
            crypted := new.password;
        else
            crypted := crypt(new.password);
        end if;
        query := query || sep
                       || ''crypt = ''
                       || quote_literal(crypted);
        sep := '', '';
    end if;
    if new.expire_interval != old.expire_interval then
        query := query || sep
                       || ''expire_date = ''
                       || quote_literal(new.billing_start
                              + interval ''1 month'' * new.expire_interval);
        sep := '', '';
    end if;
    if new.email != old.email then
        id := split_part(new.email, ''@'', 1);
        host := split_part(new.email, ''@'', 2);
        query := query || sep
                       || ''login = ''
                       || quote_literal(id)
                       || '', domain = ''
                       || quote_literal(host)
                       || '', home = ''
                       || quote_literal(host || ''/'' || id);
        sep := '', '';
    end if;
    if new.locked != old.locked then
        if new.locked = ''Y'' or new.locked = ''y'' then
            actv := ''N'';
        else
            actv := ''Y'';
        end if;
        query := query || sep
                       || ''active = ''
                       || quote_literal(actv);
        sep := '', '';
    end if;
    query := query || '' where gid = ''
                   || quote_literal(new.gid)
                   || '';'';
    if sep != '' '' then
        execute query;
    end if;
    return new;
end;
' language 'plpgsql';

drop trigger update_billing on billing;
create trigger update_billing after update on billing
    for each row execute procedure change_user();
