-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- $Id: long_xact.sql.in 2803 2008-06-05 12:09:48Z mcayland $
--
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
-- Copyright 2001-2003 Refractions Research Inc.
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--  
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


-----------------------------------------------------------------------
-- LONG TERM LOCKING
-----------------------------------------------------------------------

DROP FUNCTION UnlockRows(text);
DROP FUNCTION LockRow(text, text, text, text, timestamp);
DROP FUNCTION LockRow(text, text, text, text);
DROP FUNCTION LockRow(text, text, text);
DROP FUNCTION LockRow(text, text, text, timestamp);
DROP FUNCTION AddAuth(text);
DROP FUNCTION CheckAuth(text, text, text);
DROP FUNCTION CheckAuth(text, text);
DROP FUNCTION CheckAuthTrigger();
DROP FUNCTION GetTransactionID();
DROP FUNCTION EnableLongTransactions();
DROP FUNCTION LongTransactionsEnabled();
DROP FUNCTION DisableLongTransactions();

