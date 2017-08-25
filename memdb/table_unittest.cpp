#include "3rd_party/berkleydb/build_windows/db_cxx.h"
#include "3rd_party/gigabase/gigabase.h"
#include "base/file.h"
#include "base/file_util.h"
#include "scada/configuration/table_id.h"
#include "scada/configuration/db/dynamic_table.h"
#include "scada/configuration/db/table_impl.h"
#include "scada/configuration/structs/item.h"
#include "scada/data/db/berkley_table_accessor.h"
#include "scada/data/db/gigabase_table_accessor.h"
#include "test/test.h"
#include "tests/resource.h"
#include "tests/utils.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/*TEST_F(TableUpgrade) {
  std::string source_path = file_util::GetTempFilePath();
  std::string destination_path = file_util::GetTempFilePath();

  file_util::ScopedTempFiles temp_files;
  
  SaveResourceAsFile(MAKEINTRESOURCE(IDR_TABLE_TS), "TABLE",
      source_path.c_str());
  temp_files.Add(source_path);
  
  typedef TableImpl<data::Ts> TsTable;

  TsTable tss(data::TID_TS, "TS");

  tss.UpgradeFile(source_path, destination_path);
  ASSERT_EQ(1, tss.original_version());
  
  tss.OpenFile(destination_path.c_str());
  
  ASSERT_EQ(115, tss.record_count());
}*/

/*TEST_F(DynamicTable) {
  std::string source_path = file_util::GetTempFilePath();
  
  SaveResourceAsFile(MAKEINTRESOURCE(IDR_TABLE_TS), "TABLE",
      source_path.c_str());
      
  DynamicTable table;
  table.OpenFile(source_path.c_str());
  
  ASSERT_EQ(115, table.record_count());
}*/

/*TEST_F(GigabaseTable) {
  file_util::ScopedTempFiles temp_files;
  
  std::string path = file_util::GetTempFilePath();
  temp_files.Add(path);
  
  ::DeleteFile(path.c_str());
  
  TableImpl<data::Ts> table(data::TID_TS, "TS");
  
  dbDatabase database;

  scoped_ptr<GigabaseTableAccessor> accessor(
      new GigabaseTableAccessor(table, database));
  table.SetAccessor(accessor.detach());

  database.open(path.c_str());

  data::Ts ts;
  ts.Clear();
  ts.id = table.GenerateID();
  table.Insert(&ts);
}*/

TEST_F(BerkleyDbTable) {
  ::DeleteFile("test.db");

  DbEnv env(0);

  {
    TableImpl<data::Ts> table(data::TID_TS, "TS");
    
    scoped_ptr<BerkleyTableAccessor> accessor(
        new BerkleyTableAccessor(table, &env));
    accessor->Open("test.db");
    table.SetAccessor(accessor.detach());

    data::Ts ts;
    ts.Clear();
    ts.id = 1;
    strcpy(ts.name, "TS1");
    table.Insert(&ts);
  }

  {
    TableImpl<data::Ts> table(data::TID_TS, "TS");
    
    scoped_ptr<BerkleyTableAccessor> accessor(
        new BerkleyTableAccessor(table, &env));
    accessor->Open("test.db");
    table.SetAccessor(accessor.detach());

    const data::Ts* ts = reinterpret_cast<const data::Ts*>(table.FindRecord(1));
    ASSERT_NOT_NULL(ts);
    ASSERT_EQ(0, strcmp(ts->name, "TS1"));
  }
}
