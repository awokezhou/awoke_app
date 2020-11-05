
#include "bk_filecache.h"



static const char * const redundant_string =
	"No one would have believed in the last years of the nineteenth "
	"century that this world was being watched keenly and closely by "
	"intelligences greater than man's and yet as mortal as his own; that as "
	"men busied themselves about their various concerns they were "
	"scrutinised and studied, perhaps almost as narrowly as a man with a "
	"microscope might scrutinise the transient creatures that swarm and "
	"multiply in a drop of water.  With infinite complacency men went to "
	"and fro over this globe about their little affairs, serene in their "
	"assurance of their empire over matter. It is possible that the "
	"infusoria under the microscope do the same.  No one gave a thought to "
	"the older worlds of space as sources of human danger, or thought of "
	"them only to dismiss the idea of life upon them as impossible or "
	"improbable.  It is curious to recall some of the mental habits of "
	"those departed days.  At most terrestrial men fancied there might be "
	"other men upon Mars, perhaps inferior to themselves and ready to "
	"welcome a missionary enterprise. Yet across the gulf of space, minds "
	"that are to our minds as ours are to those of the beasts that perish, "
	"intellects vast and cool and unsympathetic, regarded this earth with "
	"envious eyes, and slowly and surely drew their plans against us.  And "
	"early in the twentieth century came the great disillusionment. \n";

static const char * const log_string = "[2020/10/16 10:00:14] [MK] [DEBUG] [mk_chunkinfo_generate_from_file:204] external call from public api\n";

bk_filecache filecache = {
	.name = "xxx.log",
	.maxsize = 64*1024,	/* 32KB */
	.cachemax = 3,
	.cachenum = 0,
	.init_finish = 0,
};

static bool fc_file_exist(const char *filepath)
{
	int r;
	FILE *fp;

	fp = fopen(filepath, "r");
	if (!fp) {
		return FALSE;
	}

	fclose(fp);
	return TRUE;
}

static fc_file_size(const char *filepath)
{
	struct stat statbuff;

	if (stat(filepath, &statbuff) < 0) {
		return -1;
	} else {
		return statbuff.st_size;
	}	
}

static void filecahce_item_add(struct _bk_filecache *fc, int id)
{
	bk_filecache_item *ni;
	char name[32] = {'\0'};
	
	ni = mem_alloc_z(sizeof(bk_filecache_item));
	if (!ni) {
		log_err("alloc filecache item error");
		return;
	}

	sprintf(name, "%s.%d", fc->name, id);
	rename(fc->name, name);
	ni->id = id;
	list_append(&ni->_head, &fc->caches);
	fc->cachenum++;
	log_trace("item:%d add to caches", id);
}

static void filecahce_init(struct _bk_filecache *fc)
{
	int i;
	char name[32] = {'\0'};
	
	/* init cache list */
	list_init(&fc->caches);

	/* record all existed cache file */
	for (i=fc->cachemax; i>0; i--) {

		memset(name, 0x0, 32);
		sprintf(name, "%s.%d", fc->name, i);
		log_trace("file:%s", name);

		if (fc_file_exist(name)) {
			log_trace("file:%s exist");
			filecahce_item_add(fc, i);
		} else {
			log_trace("file:%s not exist");
		}
	}
	
	fc->init_finish = 1;
	
	log_trace("filecache init finish");

	log_info("");
	log_info("filecahce");
	log_info("----------------");
	log_info("name:%s", fc->name);
	log_info("maxsize:%d bytes", fc->maxsize);
	log_info("cachenum:%d", fc->cachenum);
	log_info("cachemax:%d", fc->cachemax);
	log_info("----------------");
	log_info("");
}

static bool filecache_full(struct _bk_filecache *fc)
{
	int size;

	size = fc_file_size(fc->name);
	log_trace("%s size:%d", fc->name, size);

	if (size >= fc->maxsize) {
		return TRUE;
	}

	return FALSE;
}

static void filecache_dump(struct _bk_filecache *fc)
{
	bk_filecache_item *i;
	char name[32] = {'\0'};

	if (!fc->cachenum)
		return;
	
	log_debug("");
	log_debug("filecahce dump");
	log_debug("----------------");

	list_for_each_entry(i, &fc->caches, _head) {
		memset(name, 0x0, 32);
		sprintf(name, "%s.%d", fc->name, i->id);
		log_debug("file:%s", name);
	}

	log_debug("----------------");
	log_debug("");
}

static void filecahce_update(struct _bk_filecache *fc)
{
	bk_filecache_item *i;
	char name[32] = {'\0'};
	char sname[32] = {'\0'};

	/* if cache full, delete last */
	if (fc->cachenum >= fc->cachemax) {
		i = list_entry_first(&fc->caches, bk_filecache_item, _head);
		list_unlink(&i->_head);
		sprintf(name, "%s.%d", fc->name, i->id);
		log_trace("remove %s", name);
		mem_free(i);
		remove(name);
	}

	list_for_each_entry(i, &fc->caches, _head) {
		memset(name, 0x0, 32);
		sprintf(sname, "%s.%d", fc->name, i->id);
		i->id++;
		sprintf(name, "%s.%d", fc->name, i->id);
		rename(sname, name);
	}

	filecahce_item_add(fc, 1);
}

static void filecache_write(struct _bk_filecache *fc, char *message, int length)
{
	FILE *fp;

	fp = fopen(fc->name, "a+");
	if (!fp) {
		fp = fopen(fc->name, "w");
		if (!fp) {
			return;
		}
	}

	fwrite(message, length, 1, fp);
	fclose(fp);
}

void bk_filecache_write(char *message, int length)
{
	bk_filecache *fc = &filecache;

	/* check and do init */
	if (!fc->init_finish) {
		log_trace("filecache not init");
		filecahce_init(fc);
	}

	/* if file full, dump to cache */
	if (filecache_full(fc)) {
		log_trace("full");
		filecahce_update(fc);
	}

	filecache_write(fc, message, length);

	filecache_dump(fc);
}

static void *filecache_write_work(void *ctx)
{
	bk_filecache_write(log_string, strlen(log_string));
}

err_type benchmark_filecache_test(int argc, char *argv[])
{
	awoke_worker *wk = awoke_worker_create("fc-test", 200,
										   WORKER_FEAT_PERIODICITY|WORKER_FEAT_SUSPEND|WORKER_FEAT_TICK_MSEC,
								  		   filecache_write_work, NULL);
	awoke_worker_start(wk);

	sleep(60);

	awoke_worker_stop(wk);
	awoke_worker_destroy(wk);

	return et_ok;
}
