#pragma once

namespace base {
class FilePath;
}

namespace memdb {

class MemoryDB;

class DatabaseAccessor {
 public:
  explicit DatabaseAccessor(MemoryDB& database)
      : database_(database),
        just_created_(false) { }
  virtual ~DatabaseAccessor() { }
  
  bool just_created() const { return just_created_; }
  
  virtual void Open(const base::FilePath& database_path) = 0;
  
 protected:
  MemoryDB& database() const { return database_; }
  
  void set_just_created() { just_created_ = true; }
  
 private:
  MemoryDB& database_;
  bool just_created_;

  DISALLOW_COPY_AND_ASSIGN(DatabaseAccessor);
};

} // namespace memdb
