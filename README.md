# sqlite-cpp-sqlgen

Simple tool to generate (and check) sqlite code from SQL statement comments


#Examples

Insert:

```c++
/**
* @-SQLGenAccess
* @func void ServerBackupDao::addMiscValue
* @sql
*       INSERT INTO misc (tkey, tvalue) VALUES (:tkey(string), :tvalue(string))
*/
```

==>

```c++
void ServerBackupDao::addMiscValue(const std::string& tkey, const std::string& tvalue)
{
	if(q_addMiscValue==NULL)
	{
		q_addMiscValue=db->Prepare("INSERT INTO misc (tkey, tvalue) VALUES (?, ?)", false);
	}
	q_addMiscValue->Bind(tkey);
	q_addMiscValue->Bind(tvalue);
	q_addMiscValue->Write();
	q_addMiscValue->Reset();
}

class ServerBackupDao
{
	IQuery* q_addMiscValue;
public:
	void addMiscValue(const std::string& tkey, const std::string& tvalue);
};

```

Select:

```c++
/**
* @-SQLGenAccess
* @func SLastIncremental ServerBackupDao::getLastIncrementalFileBackup
* @return int incremental, string path, int resumed, int complete, int id, int incremental_ref, int deletion_protected
* @sql
*       SELECT incremental, path, resumed, complete, id, incremental_ref, deletion_protected
*		FROM backups WHERE clientid=:clientid(int) AND tgroup=:tgroup(int) AND done=1 ORDER BY backuptime DESC LIMIT 1
*/
```

==>

```c++
ServerBackupDao::SLastIncremental ServerBackupDao::getLastIncrementalFileBackup(int clientid, int tgroup)
{
	if(q_getLastIncrementalFileBackup==NULL)
	{
		q_getLastIncrementalFileBackup=db->Prepare("SELECT incremental, path, resumed, complete, id, incremental_ref, deletion_protected FROM backups WHERE clientid=? AND tgroup=? AND done=1 ORDER BY backuptime DESC LIMIT 1", false);
	}
	q_getLastIncrementalFileBackup->Bind(clientid);
	q_getLastIncrementalFileBackup->Bind(tgroup);
	db_results res=q_getLastIncrementalFileBackup->Read();
	q_getLastIncrementalFileBackup->Reset();
	SLastIncremental ret = { false, 0, "", 0, 0, 0, 0, 0 };
	if(!res.empty())
	{
		ret.exists=true;
		ret.incremental=watoi(res[0]["incremental"]);
		ret.path=res[0]["path"];
		ret.resumed=watoi(res[0]["resumed"]);
		ret.complete=watoi(res[0]["complete"]);
		ret.id=watoi(res[0]["id"]);
		ret.incremental_ref=watoi(res[0]["incremental_ref"]);
		ret.deletion_protected=watoi(res[0]["deletion_protected"]);
	}
	return ret;
}

class ServerBackupDao
{
	IQuery* q_getLastIncrementalFileBackup;
public:
	struct SLastIncremental
	{
		bool exists;
		int incremental;
		std::string path;
		int resumed;
		int complete;
		int id;
		int incremental_ref;
		int deletion_protected;
	};
	SLastIncremental getLastIncrementalFileBackup(int clientid, int tgroup);
};
```

```c++
/**
* @-SQLGenAccess
* @func int ServerBackupDao::hasFileBackups
* @return int_raw c
* @sql
*      SELECT COUNT(*) AS c FROM backups WHERE clientid=:clientid(int) AND done=1 LIMIT 1
*/
```

==>

```c++
int ServerBackupDao::hasFileBackups(int clientid)
{
	if(q_hasFileBackups==NULL)
	{
		q_hasFileBackups=db->Prepare("SELECT COUNT(*) AS c FROM backups WHERE clientid=? AND done=1 LIMIT 1", false);
	}
	q_hasFileBackups->Bind(clientid);
	db_results res=q_hasFileBackups->Read();
	q_hasFileBackups->Reset();
	assert(!res.empty());
	return watoi(res[0]["c"]);
}

class ServerBackupDao
{
	IQuery* q_getLastIncrementalFileBackup;
public:
	int hasFileBackups(int clientid);
};
```

See also e.g. https://github.com/uroni/urbackup_backend/blob/dev/urbackupserver/dao/ServerBackupDao.cpp