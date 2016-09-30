/* Copyright (c) 2016 Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include "dd/info_schema/show.h"

#include <string.h>
#include <string>

#include "dd/info_schema/show_query_builder.h" // Select_lex_builder
#include "dd/info_schema/stats.h"
#include "m_string.h"
#include "mdl.h"
#include "my_sqlcommand.h"
#include "mysqld.h"
#include "session_tracker.h"
#include "sql_class.h"
#include "sql_lex.h"
#include "sql_list.h"
#include "sql_string.h"
#include "table.h"
#include "thr_lock.h"

namespace dd {
namespace info_schema {

//  Build a substitue query for SHOW CHARSETS.

SELECT_LEX*
build_show_character_set_query(const POS &pos,
                               THD *thd,
                               const String *wild,
                               Item *where_cond)
{
  static const LEX_STRING system_view_name= {
    C_STRING_WITH_LEN("CHARACTER_SETS") };

  // Define field name literal used in query to be built.
  static const LEX_STRING field_charset= {
    C_STRING_WITH_LEN("CHARACTER_SET_NAME") };
  static const LEX_STRING alias_charset= {
    C_STRING_WITH_LEN("Charset") };

  static const LEX_STRING field_desc= {
    C_STRING_WITH_LEN("DESCRIPTION") };
  static const LEX_STRING alias_desc= {
    C_STRING_WITH_LEN("Description") };

  static const LEX_STRING field_collate= {
    C_STRING_WITH_LEN("DEFAULT_COLLATE_NAME") };
  static const LEX_STRING alias_collate= {
    C_STRING_WITH_LEN("Default collation") };

  static const LEX_STRING field_maxlen= {
    C_STRING_WITH_LEN("MAXLEN") };
  static const LEX_STRING alias_maxlen= {
    C_STRING_WITH_LEN("Maxlen") };

  /*
     Build sub query.

     ...
     SELECT
       CHARACTER_SET_NAME as Charset,
       DESCRIPTION as Description,
       DEFAULT_COLLATE_NAME as `Default collation`,
       MAXLEN as `Maxlen`
     ...
  */
  Select_lex_builder sub_query(&pos, thd);
  if (sub_query.add_select_item(field_charset, alias_charset) ||
      sub_query.add_select_item(field_desc, alias_desc) ||
      sub_query.add_select_item(field_collate, alias_collate) ||
      sub_query.add_select_item(field_maxlen, alias_maxlen))
    return nullptr;

  // ... FROM information_schema.<table_name> ...
  if (sub_query.add_from_item(INFORMATION_SCHEMA_NAME,
                              system_view_name))
    return nullptr;

  /*
    Build the top level query
  */

  Select_lex_builder top_query(&pos, thd);

  // SELECT * FROM <sub_query> ...
  if (top_query.add_star_select_item() ||
      top_query.add_from_item(
                  sub_query.prepare_derived_table(system_view_name)))
    return nullptr;

  // SELECT * FROM <sub_query> WHERE Charset LIKE <value> ...
  if (wild)
  {
    Item *like= top_query.prepare_like_item(alias_charset, wild);
    if (!like || top_query.add_condition(like))
      return nullptr;
  }
  else if (where_cond && top_query.add_condition(where_cond))
    return nullptr;

  // ... ORDER BY 'Charset' ...
  if (top_query.add_order_by(alias_charset))
    return nullptr;

  SELECT_LEX* sl= top_query.prepare_select_lex();

  // sql_command is set to SQL_QUERY after above call, so.
  thd->lex->sql_command= SQLCOM_SHOW_CHARSETS;

  return sl;
}


// Build a substitue query for SHOW COLLATION.

SELECT_LEX*
build_show_collation_query(const POS &pos,
                           THD *thd,
                           const String *wild,
                           Item *where_cond)
{
  static const LEX_STRING system_view_name= {
    C_STRING_WITH_LEN("COLLATIONS") };

  // Define field name literal used in query to be built.
  static const LEX_STRING field_collation= {
    C_STRING_WITH_LEN("COLLATION_NAME") };
  static const LEX_STRING alias_collation= {
    C_STRING_WITH_LEN("Collation") };

  static const LEX_STRING field_charset= {
    C_STRING_WITH_LEN("CHARACTER_SET_NAME") };
  static const LEX_STRING alias_charset= {
    C_STRING_WITH_LEN("Charset") };

  static const LEX_STRING field_id= { C_STRING_WITH_LEN("ID") };
  static const LEX_STRING alias_id= { C_STRING_WITH_LEN("Id") };

  static const LEX_STRING field_default= {
    C_STRING_WITH_LEN("IS_DEFAULT") };
  static const LEX_STRING alias_default= {
    C_STRING_WITH_LEN("Default") };

  static const LEX_STRING field_compiled= {
    C_STRING_WITH_LEN("IS_COMPILED") };
  static const LEX_STRING alias_compiled= {
    C_STRING_WITH_LEN("Compiled") };

  static const LEX_STRING field_sortlen= {
    C_STRING_WITH_LEN("SORTLEN") };
  static const LEX_STRING alias_sortlen= {
    C_STRING_WITH_LEN("Sortlen") };

  /*
     Build sub query.

     ...
      SELECT COLLATION_NAME as `Collation`,
             CHARACTER_SET_NAME as `Charset`,
             ID as `Id`,
             IS_COMPILED as `Compiled`,
             SORTLEN as `Sortlen`
     ...
  */
  Select_lex_builder sub_query(&pos, thd);
  if (sub_query.add_select_item(field_collation, alias_collation) ||
      sub_query.add_select_item(field_charset, alias_charset) ||
      sub_query.add_select_item(field_id, alias_id) ||
      sub_query.add_select_item(field_default, alias_default) ||
      sub_query.add_select_item(field_compiled, alias_compiled) ||
      sub_query.add_select_item(field_sortlen, alias_sortlen))
    return nullptr;

  // ... FROM information_schema.<table_name> ...
  if (sub_query.add_from_item(INFORMATION_SCHEMA_NAME,
                              system_view_name))
    return nullptr;

  /*
    Build the top level query
  */

  Select_lex_builder top_query(&pos, thd);

  // SELECT * FROM <sub_query> ...
  if (top_query.add_star_select_item() ||
      top_query.add_from_item(
                  sub_query.prepare_derived_table(system_view_name)))
    return nullptr;

  // SELECT * FROM <sub_query> WHERE Collation LIKE <value> ...
  if (wild)
  {
    Item *like= top_query.prepare_like_item(alias_collation, wild);
    if (!like || top_query.add_condition(like))
      return nullptr;
  } else if (where_cond && top_query.add_condition(where_cond))
    return nullptr;

  // ... ORDER BY 'Collation' ...
  if (top_query.add_order_by(alias_collation))
    return nullptr;

  SELECT_LEX* sl= top_query.prepare_select_lex();

  // sql_command is set to SQL_QUERY after above call, so.
  thd->lex->sql_command= SQLCOM_SHOW_COLLATIONS;

  return sl;
}


// Build a substitue query for SHOW DATABASES.

SELECT_LEX*
build_show_databases_query(const POS &pos,
                           THD *thd,
                           String *wild,
                           Item *where_cond)
{
  static const LEX_STRING system_view_name= {
    C_STRING_WITH_LEN("SCHEMATA") };

  // Define field name literal used in query to be built.
  static const LEX_STRING field_schema_name= {
    C_STRING_WITH_LEN("SCHEMA_NAME") };
  static const LEX_STRING alias_database= {
    C_STRING_WITH_LEN("Database") };

  // Build the alias 'Database (<dbname>%)'
  String_type alias;
  alias.append(alias_database.str);
  if (wild)
  {
    alias.append(" (");
    alias.append(wild->ptr(), wild->length());
    alias.append(")");
  }
  char *tmp= static_cast<char*> (thd->alloc(alias.length()+1));
  memcpy(tmp, alias.c_str(), alias.length());
  *(tmp+alias.length())='\0';
  LEX_STRING alias_lex_string= { tmp, alias.length() };

  /*
     Build sub query.

     ...
       SELECT SCHEMA_NAME as `Database`,
          FROM information_schema.schemata;
     ...
  */
  Select_lex_builder sub_query(&pos, thd);
  if (sub_query.add_select_item(field_schema_name, alias_database) ||
      sub_query.add_from_item(INFORMATION_SCHEMA_NAME,
                              system_view_name))
    return nullptr;

  /*
    Build the top level query
  */

  Select_lex_builder top_query(&pos, thd);

  // SELECT * FROM <sub_query> ...
  if (top_query.add_select_item(alias_database, alias_lex_string) ||
      top_query.add_from_item(
                  sub_query.prepare_derived_table(system_view_name)))
    return nullptr;

  // SELECT * FROM <sub_query> WHERE Database LIKE <value> ...
  if (wild)
  {
    // Convert IS db and table name to desired form.
    dd::info_schema::convert_table_name_case(const_cast<char *>(wild->ptr()),
                                             nullptr);

    Item *like= top_query.prepare_like_item(alias_database, wild);
    if (!like || top_query.add_condition(like))
      return nullptr;
  } else if (where_cond && top_query.add_condition(where_cond))
    return nullptr;

  // ... ORDER BY 'Database' ...
  if (top_query.add_order_by(alias_database))
    return nullptr;

  SELECT_LEX* sl= top_query.prepare_select_lex();

  // sql_command is set to SQL_QUERY after above call, so.
  thd->lex->sql_command= SQLCOM_SHOW_DATABASES;

  return sl;
}


/**
  Add fields required by SHOW TABLE STATUS.

  @param query - Select_lex_builder to which we add fields.
  @param alias_as_alias - Add select items of the form,
                          'alias AS alias'. This is required
                          for top level query, as the real field
                          names would be valied only the sub
                          query that we are building for SHOW
                          TABLE STATUS. Check comments in
                          sql/dd/show.h for more details.

  @returns false on success.
           true on failure.
*/

static bool add_table_status_fields(Select_lex_builder *query,
                                    bool alias_as_alias)
{
  // Field for SHOW TABLE STATUS
  static const LEX_STRING field_engine = {
    C_STRING_WITH_LEN("ENGINE") };
  static const LEX_STRING alias_engine = {
    C_STRING_WITH_LEN("Engine") };

  static const LEX_STRING field_version = {
    C_STRING_WITH_LEN("VERSION") };
  static const LEX_STRING alias_version = {
    C_STRING_WITH_LEN("Version") };

  static const LEX_STRING field_row_format = {
    C_STRING_WITH_LEN("ROW_FORMAT") };
  static const LEX_STRING alias_row_format = {
    C_STRING_WITH_LEN("Row_format") };

  static const LEX_STRING field_rows = {
    C_STRING_WITH_LEN("TABLE_ROWS") };
  static const LEX_STRING alias_rows = { C_STRING_WITH_LEN("Rows") };

  static const LEX_STRING field_avg_row_length = {
    C_STRING_WITH_LEN("AVG_ROW_LENGTH") };
  static const LEX_STRING alias_avg_row_length = {
    C_STRING_WITH_LEN("Avg_row_length") };

  static const LEX_STRING field_data_length = {
    C_STRING_WITH_LEN("DATA_LENGTH") };
  static const LEX_STRING alias_data_length = {
    C_STRING_WITH_LEN("Data_length") };

  static const LEX_STRING field_max_data_length = {
    C_STRING_WITH_LEN("MAX_DATA_LENGTH") };
  static const LEX_STRING alias_max_data_length = {
    C_STRING_WITH_LEN("Max_data_length") };

  static const LEX_STRING field_index_length = {
    C_STRING_WITH_LEN("INDEX_LENGTH") };
  static const LEX_STRING alias_index_length = {
    C_STRING_WITH_LEN("Index_length") };

  static const LEX_STRING field_data_free = {
    C_STRING_WITH_LEN("DATA_FREE") };
  static const LEX_STRING alias_data_free = {
    C_STRING_WITH_LEN("Data_free") };

  static const LEX_STRING field_auto_increment = {
    C_STRING_WITH_LEN("AUTO_INCREMENT") };
  static const LEX_STRING alias_auto_increment = {
    C_STRING_WITH_LEN("Auto_increment") };

  static const LEX_STRING field_create_time = {
    C_STRING_WITH_LEN("CREATE_TIME") };
  static const LEX_STRING alias_create_time = {
    C_STRING_WITH_LEN("Create_time") };

  static const LEX_STRING field_update_time = {
    C_STRING_WITH_LEN("UPDATE_TIME") };
  static const LEX_STRING alias_update_time = {
    C_STRING_WITH_LEN("Update_time") };

  static const LEX_STRING field_check_time = {
    C_STRING_WITH_LEN("CHECK_TIME") };
  static const LEX_STRING alias_check_time = {
    C_STRING_WITH_LEN("Check_time") };

  static const LEX_STRING field_collation = {
    C_STRING_WITH_LEN("TABLE_COLLATION") };
  static const LEX_STRING alias_collation = {
    C_STRING_WITH_LEN("Collation") };

  static const LEX_STRING field_checksum = {
    C_STRING_WITH_LEN("CHECKSUM") };
  static const LEX_STRING alias_checksum = {
    C_STRING_WITH_LEN("Checksum") };

  static const LEX_STRING field_create_options = {
    C_STRING_WITH_LEN("CREATE_OPTIONS") };
  static const LEX_STRING alias_create_options = {
    C_STRING_WITH_LEN("Create_options") };

  static const LEX_STRING field_comment = {
    C_STRING_WITH_LEN("TABLE_COMMENT") };
  static const LEX_STRING alias_comment = {
    C_STRING_WITH_LEN("Comment") };

  if (alias_as_alias == false)
  {
    if (query->add_select_item(field_engine, alias_engine) ||
        query->add_select_item(field_version, alias_version) ||
        query->add_select_item(field_row_format, alias_row_format) ||
        query->add_select_item(field_rows, alias_rows) ||
        query->add_select_item(field_avg_row_length, alias_avg_row_length) ||
        query->add_select_item(field_data_length, alias_data_length) ||
        query->add_select_item(field_max_data_length, alias_max_data_length) ||
        query->add_select_item(field_index_length, alias_index_length) ||
        query->add_select_item(field_data_free, alias_data_free) ||
        query->add_select_item(field_auto_increment, alias_auto_increment) ||
        query->add_select_item(field_create_time, alias_create_time) ||
        query->add_select_item(field_update_time, alias_update_time) ||
        query->add_select_item(field_check_time, alias_check_time) ||
        query->add_select_item(field_collation, alias_collation) ||
        query->add_select_item(field_checksum, alias_checksum) ||
        query->add_select_item(field_create_options, alias_create_options) ||
        query->add_select_item(field_comment, alias_comment))
      return true;
  }
  else
  {
    if (query->add_select_item(alias_engine, alias_engine) ||
        query->add_select_item(alias_version, alias_version) ||
        query->add_select_item(alias_row_format, alias_row_format) ||
        query->add_select_item(alias_rows, alias_rows) ||
        query->add_select_item(alias_avg_row_length, alias_avg_row_length) ||
        query->add_select_item(alias_data_length, alias_data_length) ||
        query->add_select_item(alias_max_data_length, alias_max_data_length) ||
        query->add_select_item(alias_index_length, alias_index_length) ||
        query->add_select_item(alias_data_free, alias_data_free) ||
        query->add_select_item(alias_auto_increment, alias_auto_increment) ||
        query->add_select_item(alias_create_time, alias_create_time) ||
        query->add_select_item(alias_update_time, alias_update_time) ||
        query->add_select_item(alias_check_time, alias_check_time) ||
        query->add_select_item(alias_collation, alias_collation) ||
        query->add_select_item(alias_checksum, alias_checksum) ||
        query->add_select_item(alias_create_options, alias_create_options) ||
        query->add_select_item(alias_comment, alias_comment))
      return true;
  }

  return false;
}


// Build a substitue query for SHOW TABLES / TABLE STATUS.

SELECT_LEX*
build_show_tables_query(const POS &pos,
                        THD *thd,
                        String *wild,
                        Item *where_cond,
                        bool include_status_fields)
{
  static const LEX_STRING system_view_name= {
    C_STRING_WITH_LEN("TABLES") };

  // Define field name literal used in query to be built.
  static const LEX_STRING field_table= {
    C_STRING_WITH_LEN("TABLE_NAME") };
  static const LEX_STRING alias_table= { C_STRING_WITH_LEN("Name") };

  static const LEX_STRING field_database= {
    C_STRING_WITH_LEN("TABLE_SCHEMA") };
  static const LEX_STRING alias_database= {
    C_STRING_WITH_LEN("Database") };

  static const LEX_STRING field_table_type= {
    C_STRING_WITH_LEN("TABLE_TYPE") };
  static const LEX_STRING alias_table_type= {
    C_STRING_WITH_LEN("Table_type") };

  // Get the current logged in schema name
  size_t dummy;
  if (thd->lex->select_lex->db == nullptr &&
      thd->lex->copy_db_to(&thd->lex->select_lex->db, &dummy))
    return nullptr;

  // Convert IS db and table name to desired form.
  dd::info_schema::convert_table_name_case(
    thd->lex->select_lex->db,
    wild ? const_cast<char*>(wild->ptr()) : nullptr);

  LEX_STRING cur_db= { thd->lex->select_lex->db,
                       strlen(thd->lex->select_lex->db) };

  /*
    Build the alias 'Tables_in_<dbname> %' we are building
    SHOW TABLES and not SHOW TABLE STATUS
  */
  String_type alias;

  if (include_status_fields)
  {
    // Set the output alias for first column of SHOW TABLE STATUS as 'Name'
    alias.append(alias_table.str);
  }
  else
  {
    // Build the output alias for SHOW TABLES
    alias.append("Tables_in_");
    alias.append(cur_db.str);
    if (wild)
    {
      alias.append(" (");
      alias.append(wild->ptr());
      alias.append(")");
    }
  }
  char *tmp= static_cast<char*> (thd->alloc(alias.length()+1));
  memcpy(tmp, alias.c_str(), alias.length());
  *(tmp+alias.length())='\0';
  LEX_STRING alias_lex_string= { tmp, alias.length() };

  /*
     Build sub query.
     ...
  */
  Select_lex_builder sub_query(&pos, thd);
  if (sub_query.add_select_item(field_database, alias_database) ||
      sub_query.add_select_item(field_table, alias_lex_string) ||
      (include_status_fields == true &&
       add_table_status_fields(&sub_query, false)) ||
      (include_status_fields == false &&
       thd->lex->verbose && sub_query.add_select_item(field_table_type,
                                                      alias_table_type)))
    return nullptr;

  // ... FROM information_schema.tables ...
  if (sub_query.add_from_item(INFORMATION_SCHEMA_NAME,
                              system_view_name))
      return nullptr;

  /*
    Build the top level query
  */

  Select_lex_builder top_query(&pos, thd);

  // SELECT * FROM <sub_query> ...
  if (top_query.add_select_item(alias_lex_string, alias_lex_string) ||
      (include_status_fields == true &&
       add_table_status_fields(&top_query, true)) ||
      (include_status_fields == false &&
       thd->lex->verbose && top_query.add_select_item(alias_table_type,
                                                      alias_table_type)) ||
      top_query.add_from_item(
        sub_query.prepare_derived_table(system_view_name)))
    return nullptr;

  // ... WHERE 'Database' = <dbname> ...
  Item *database_condition=
    top_query.prepare_equal_item(alias_database, cur_db);
  if (top_query.add_condition(database_condition))
    return nullptr;

  // ... [ AND ] Table LIKE <value> ...
  if (wild)
  {
    Item *like= top_query.prepare_like_item(alias_lex_string, wild);
    if (!like || top_query.add_condition(like))
      return nullptr;
  }

  // ... [ AND ] <user provided condition> ...
  if (where_cond && top_query.add_condition(where_cond))
    return nullptr;

  // ... ORDER BY 'Table' ...
  if (top_query.add_order_by(alias_lex_string))
    return nullptr;

  SELECT_LEX* sl= top_query.prepare_select_lex();

  // sql_command is set to SQL_QUERY after above call, so.
  if (include_status_fields)
    thd->lex->sql_command= SQLCOM_SHOW_TABLE_STATUS;
  else
    thd->lex->sql_command= SQLCOM_SHOW_TABLES;

  return sl;
}


// Build a substitue query for SHOW COLUMNS/FIELDS OR DESCRIBE.

SELECT_LEX*
build_show_columns_query(const POS &pos,
                         THD *thd,
                         Table_ident *table_ident,
                         const String *wild,
                         Item *where_cond)
{
  static const LEX_STRING system_view_name= {
    C_STRING_WITH_LEN("COLUMNS") };

  // Define field name literal used in query to be built.
  static const LEX_STRING field_database= {
    C_STRING_WITH_LEN("TABLE_SCHEMA") };
  static const LEX_STRING alias_database= {
    C_STRING_WITH_LEN("Database") };

  static const LEX_STRING field_table= {
    C_STRING_WITH_LEN("TABLE_NAME") };
  static const LEX_STRING alias_table= { C_STRING_WITH_LEN("Table") };

  static const LEX_STRING field_field= {
    C_STRING_WITH_LEN("COLUMN_NAME") };
  static const LEX_STRING alias_field= { C_STRING_WITH_LEN("Field") };

  static const LEX_STRING field_column_type= {
    C_STRING_WITH_LEN("COLUMN_TYPE") };
  static const LEX_STRING alias_column_type= {
    C_STRING_WITH_LEN("Type") };

  static const LEX_STRING field_collation= {
    C_STRING_WITH_LEN("COLLATION_NAME") };
  static const LEX_STRING alias_collation= {
    C_STRING_WITH_LEN("Collation") };

  static const LEX_STRING field_null= {
    C_STRING_WITH_LEN("IS_NULLABLE") };
  static const LEX_STRING alias_null= { C_STRING_WITH_LEN("Null") };

  static const LEX_STRING field_key= {
    C_STRING_WITH_LEN("COLUMN_KEY") };
  static const LEX_STRING alias_key= { C_STRING_WITH_LEN("Key") };

  static const LEX_STRING field_default= {
    C_STRING_WITH_LEN("COLUMN_DEFAULT") };
  static const LEX_STRING alias_default= {
    C_STRING_WITH_LEN("Default") };

  static const LEX_STRING field_extra= {
    C_STRING_WITH_LEN("EXTRA") };
  static const LEX_STRING alias_extra= { C_STRING_WITH_LEN("Extra") };

  static const LEX_STRING field_privileges= {
    C_STRING_WITH_LEN("PRIVILEGES") };
  static const LEX_STRING alias_privileges= {
    C_STRING_WITH_LEN("Privileges") };

  static const LEX_STRING field_comment= {
    C_STRING_WITH_LEN("COLUMN_COMMENT") };
  static const LEX_STRING alias_comment= {
    C_STRING_WITH_LEN("Comment") };

  static const LEX_STRING field_ordinal_position= {
    C_STRING_WITH_LEN("ORDINAL_POSITION") };
  static const LEX_STRING alias_ordinal_position= {
    C_STRING_WITH_LEN("Ordinal_position") };

  // Get the current logged in schema name
  LEX_STRING cur_db;
  if (table_ident->db.str)
  {
    cur_db.str= (char *)table_ident->db.str;
    cur_db.length= table_ident->db.length;
  }
  else if (thd->lex->copy_db_to(&cur_db.str, &cur_db.length))
    return nullptr;

  // Convert IS db and table name to desired form.
  dd::info_schema::convert_table_name_case(cur_db.str,
                     const_cast<char*>(table_ident->table.str));

  /*
     Build sub query.
     ...
  */
  Select_lex_builder sub_query(&pos, thd);
  if (sub_query.add_select_item(field_database, alias_database) ||
      sub_query.add_select_item(field_table, alias_table) ||
      sub_query.add_select_item(field_field, alias_field) ||
      sub_query.add_select_item(field_column_type, alias_column_type) ||
      (thd->lex->verbose == true &&
       sub_query.add_select_item(field_collation, alias_collation)) ||
      sub_query.add_select_item(field_null, alias_null) ||
      sub_query.add_select_item(field_key, alias_key) ||
      sub_query.add_select_item(field_default, alias_default) ||
      sub_query.add_select_item(field_extra, alias_extra) ||
      (thd->lex->verbose == true &&
       sub_query.add_select_item(field_privileges, alias_privileges)) ||
      (thd->lex->verbose == true &&
       sub_query.add_select_item(field_comment, alias_comment)) ||
      sub_query.add_select_item(field_ordinal_position, alias_ordinal_position))
    return nullptr;

  // ... FROM information_schema.columns ...
  if (sub_query.add_from_item(INFORMATION_SCHEMA_NAME,
                              system_view_name))
      return nullptr;

  /*
    Build the top level query
  */

  Select_lex_builder top_query(&pos, thd);

  // SELECT * FROM <sub_query> ...
  if (top_query.add_select_item(alias_field, alias_field) ||
      top_query.add_select_item(alias_column_type, alias_column_type) ||
      (thd->lex->verbose == true &&
       top_query.add_select_item(alias_collation, alias_collation)) ||
      top_query.add_select_item(alias_null, alias_null) ||
      top_query.add_select_item(alias_key, alias_key) ||
      top_query.add_select_item(alias_default, alias_default) ||
      top_query.add_select_item(alias_extra, alias_extra) ||
      (thd->lex->verbose == true &&
       top_query.add_select_item(alias_privileges, alias_privileges)) ||
      (thd->lex->verbose == true &&
       top_query.add_select_item(alias_comment, alias_comment)) ||
      top_query.add_from_item(
                  sub_query.prepare_derived_table(system_view_name)))
    return nullptr;

  // ... WHERE 'Database' = <dbname> ...
  Item *database_condition=
    top_query.prepare_equal_item(alias_database, cur_db);
  if (!database_condition || top_query.add_condition(database_condition))
    return nullptr;

  // ... AND Table = table_ident->table->str ...
  LEX_STRING tmp_lex_string= {
    const_cast<char*>(table_ident->table.str),
    table_ident->table.length };
  Item *table_condition=
    top_query.prepare_equal_item(alias_table, tmp_lex_string);
  if (!table_condition || top_query.add_condition(table_condition))
    return nullptr;

  // ... [ AND ] Field LIKE <value> ...
  if (wild)
  {
    Item *like= top_query.prepare_like_item(alias_field, wild);
    if (!like || top_query.add_condition(like))
      return nullptr;
  }

  // ... [ AND ] <user provided condition> ...
  if (where_cond && top_query.add_condition(where_cond))
    return nullptr;

  // ... ORDER BY 'Ordinal_position' ...
  if (top_query.add_order_by(alias_ordinal_position))
    return nullptr;

  /*
    Add table_ident to schema_select_lex, this is used later to
    validate if the given table exists in select_precheck()
  */
  TABLE_LIST **query_tables_last= thd->lex->query_tables_last;
  SELECT_LEX *schema_select_lex= nullptr;
  if ((schema_select_lex= thd->lex->new_empty_query_block()) == nullptr)
    return nullptr;
  if (!schema_select_lex->add_table_to_list(thd, table_ident, 0, 0, TL_READ,
                                            MDL_SHARED_READ))
    return nullptr;
  thd->lex->query_tables_last= query_tables_last;

  // Prepare the SELECT_LEX
  SELECT_LEX* sl= top_query.prepare_select_lex();
  if (sl == nullptr)
    return nullptr;

  // sql_command is set to SQL_QUERY after above call, so.
  thd->lex->sql_command= SQLCOM_SHOW_FIELDS;

  // Hold the schema_select_lex in the first table_list.
  TABLE_LIST *table_list= sl->table_list.first;
  table_list->schema_select_lex= schema_select_lex;

  return sl;
}


// Build a substitue query for SHOW INDEX|KEYS|INDEXES

SELECT_LEX*
build_show_keys_query(const POS &pos,
                      THD *thd,
                      Table_ident *table_ident,
                      Item *where_cond)
{
  LEX_STRING system_view_name= { C_STRING_WITH_LEN("SHOW_STATISTICS") };

  // Define field name literal used in query to be built.

  static const LEX_STRING alias_database= {
    C_STRING_WITH_LEN("Database") };

  static const LEX_STRING alias_table= { C_STRING_WITH_LEN("Table") };

  static const LEX_STRING alias_non_unique= {
    C_STRING_WITH_LEN("Non_unique") };

  static const LEX_STRING alias_key= { C_STRING_WITH_LEN("Key_name") };

  static const LEX_STRING alias_seq_in_index= {
    C_STRING_WITH_LEN("Seq_in_index") };

  static const LEX_STRING alias_column_name= {
    C_STRING_WITH_LEN("Column_name") };

  static const LEX_STRING alias_collation= {
    C_STRING_WITH_LEN("Collation") };

  static const LEX_STRING alias_cardinality= {
    C_STRING_WITH_LEN("Cardinality") };

  static const LEX_STRING alias_sub_part= {
    C_STRING_WITH_LEN("Sub_part") };

  static const LEX_STRING alias_packed= {
    C_STRING_WITH_LEN("Packed") };

  static const LEX_STRING alias_null= { C_STRING_WITH_LEN("Null") };

  static const LEX_STRING alias_type= {
    C_STRING_WITH_LEN("Index_type") };

  static const LEX_STRING alias_comment= {
    C_STRING_WITH_LEN("Comment") };

  static const LEX_STRING alias_index_comment= {
    C_STRING_WITH_LEN("Index_comment") };

  static const LEX_STRING alias_visible= {
    C_STRING_WITH_LEN("Visible") };

  static const LEX_STRING alias_index_pos= {
    C_STRING_WITH_LEN("INDEX_ORDINAL_POSITION") };

  static const LEX_STRING alias_column_pos= {
    C_STRING_WITH_LEN("COLUMN_ORDINAL_POSITION") };

  // Get the current logged in schema name
  LEX_STRING cur_db;
  if (table_ident->db.str)
  {
    cur_db.str= (char *)table_ident->db.str;
    cur_db.length= table_ident->db.length;
  }
  else if (thd->lex->copy_db_to(&cur_db.str, &cur_db.length))
    return nullptr;

  // Convert IS db and table name to desired form.
  dd::info_schema::convert_table_name_case(cur_db.str,
                     const_cast<char*>(table_ident->table.str));

  /*
     Build sub query.
     ...
  */
  Select_lex_builder sub_query(&pos, thd);
  if (sub_query.add_select_item(alias_database, alias_database) ||
      sub_query.add_select_item(alias_table, alias_table) ||
      sub_query.add_select_item(alias_non_unique, alias_non_unique) ||
      sub_query.add_select_item(alias_key, alias_key) ||
      sub_query.add_select_item(alias_seq_in_index, alias_seq_in_index) ||
      sub_query.add_select_item(alias_column_name, alias_column_name) ||
      sub_query.add_select_item(alias_collation, alias_collation) ||
      sub_query.add_select_item(alias_cardinality, alias_cardinality) ||
      sub_query.add_select_item(alias_sub_part, alias_sub_part) ||
      sub_query.add_select_item(alias_packed, alias_packed) ||
      sub_query.add_select_item(alias_null, alias_null) ||
      sub_query.add_select_item(alias_type, alias_type) ||
      sub_query.add_select_item(alias_comment, alias_comment) ||
      sub_query.add_select_item(alias_index_comment, alias_index_comment) ||
      sub_query.add_select_item(alias_visible, alias_visible) ||
      sub_query.add_select_item(alias_index_pos, alias_index_pos) ||
      sub_query.add_select_item(alias_column_pos, alias_column_pos))
    return nullptr;

  // ... FROM information_schema.columns ...
  if (sub_query.add_from_item(INFORMATION_SCHEMA_NAME,
                              system_view_name))
      return nullptr;

  /*
    Build the top level query
  */

  Select_lex_builder top_query(&pos, thd);

  // SELECT * FROM <sub_query> ...
  if (top_query.add_select_item(alias_table, alias_table) ||
      top_query.add_select_item(alias_non_unique, alias_non_unique) ||
      top_query.add_select_item(alias_key, alias_key) ||
      top_query.add_select_item(alias_seq_in_index, alias_seq_in_index) ||
      top_query.add_select_item(alias_column_name, alias_column_name) ||
      top_query.add_select_item(alias_collation, alias_collation) ||
      top_query.add_select_item(alias_cardinality, alias_cardinality) ||
      top_query.add_select_item(alias_sub_part, alias_sub_part) ||
      top_query.add_select_item(alias_packed, alias_packed) ||
      top_query.add_select_item(alias_null, alias_null) ||
      top_query.add_select_item(alias_type, alias_type) ||
      top_query.add_select_item(alias_comment, alias_comment) ||
      top_query.add_select_item(alias_index_comment, alias_index_comment) ||
      top_query.add_select_item(alias_visible, alias_visible) ||
      top_query.add_from_item(
                  sub_query.prepare_derived_table(system_view_name)))
    return nullptr;

  // ... WHERE 'Database' = <dbname> ...
  Item *database_condition=
    top_query.prepare_equal_item(alias_database, cur_db);
  if (!database_condition || top_query.add_condition(database_condition))
    return nullptr;

  // ... AND Table = table_ident->table->str ...
  LEX_STRING tmp_lex_string= {
    const_cast<char*>(table_ident->table.str),
    table_ident->table.length };
  Item *table_condition=
    top_query.prepare_equal_item(alias_table, tmp_lex_string);
  if (!table_condition || top_query.add_condition(table_condition))
    return nullptr;

  // ... [ AND ] <user provided condition> ...
  if (where_cond && top_query.add_condition(where_cond))
    return nullptr;

  // ... ORDER BY 'INDEX_ORDINAL_POSITION, COLUMN_ORDINAL_POSITION' ...
  if (top_query.add_order_by(alias_index_pos) ||
      top_query.add_order_by(alias_column_pos))
    return nullptr;

  /*
    Add table_ident to schema_select_lex, this is used later to
    validate if the given table exists in select_precheck()
  */
  TABLE_LIST **query_tables_last= thd->lex->query_tables_last;
  SELECT_LEX *schema_select_lex= nullptr;
  if ((schema_select_lex= thd->lex->new_empty_query_block()) == nullptr)
    return nullptr;
  if (!schema_select_lex->add_table_to_list(thd, table_ident, 0, 0, TL_READ,
                                            MDL_SHARED_READ))
    return nullptr;
  thd->lex->query_tables_last= query_tables_last;

  // Prepare the SELECT_LEX
  SELECT_LEX* sl= top_query.prepare_select_lex();
  if (sl == nullptr)
    return nullptr;

  // sql_command is set to SQL_QUERY after above call, so.
  thd->lex->sql_command= SQLCOM_SHOW_KEYS;

  // Hold the schema_select_lex in the first table_list.
  TABLE_LIST *table_list= sl->table_list.first;
  table_list->schema_select_lex= schema_select_lex;

  return sl;
}

} // namespace info_schema
} // namespace dd

