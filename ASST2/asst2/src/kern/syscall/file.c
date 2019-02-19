#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <limits.h>
#include <lib.h>
#include <uio.h>
#include <thread.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <file.h>
#include <proc.h>
#include <syscall.h>
#include <copyinout.h>
#include <machine/trapframe.h>
/*
 * Add your file-related functions here ...
 */

// TODO: Take in 3rd argument register and give to vfs_open (mode_t)
// Although this is not even implemented in OS161 so whatever...

static int open_file_cnt=0;
int dup2_helper(int oldfd,int newfd,int cnt);
int seek_helper(int seek_cr);
int write_util(int filehandler,int max_size);

int sys_open(userptr_t filename, int flags, int *ret) {
	size_t got;
	int i=3,result,err;
	if (filename == NULL) {
		return EFAULT;
	}
	char *kfilename = kmalloc((PATH_MAX)*sizeof(char));
	if (kfilename == NULL) {
		return ENFILE;
	}
	result = copyinstr(filename, kfilename, PATH_MAX, &got);
	if (result) {
		kfree(kfilename);
		return result;
	}
	for (; i < MAX_PROCESS_OPEN_FILES; i++) {
		if (curproc->file_table[i] == NULL) {
			break;
		}
	}
	if (i == MAX_PROCESS_OPEN_FILES) {
		kfree(kfilename);
		return EMFILE;
	}
	if (open_file_cnt>=MAX_SYSTEM_OPEN_FILES)
	{
		kfree(kfilename);
		return ENFILE;
	}
    char *fn=kfilename;
    int descriptor=i;
	struct File *file = kmalloc(sizeof(struct File*));
	int rslt;
	struct vnode *vn;
	if(!file){
		err= ENFILE;
	}
 
	rslt = vfs_open(fn, flags, 0, &vn);
	if (rslt) {
		kfree(file);
		err= rslt;
	}

	file->flock = lock_create("lock create");
	if(!file->flock) {
		vfs_close(file->v_ptr);
		kfree(file);
		err= ENFILE;
	}
	else{
		err=0;
	}
	file->offset = 0;
	file->open_flags = flags;
	file->references = 1;
	file->v_ptr=vn;
	curproc->file_table[descriptor] = file;


	if(err){
		kfree(kfilename);
		return err;
	}

	*ret = i;
	open_file_cnt++;
	return 0;
}



int sys_close(int filehandler) { 
	struct proc *proc=curproc;
	if(filehandler < 0 || filehandler >= MAX_PROCESS_OPEN_FILES || !proc->file_table[filehandler]) {
		return EBADF;
	}

	struct File *file = proc->file_table[filehandler];

	lock_acquire(file->flock);
	proc->file_table[filehandler] = NULL;
	file->references --;
	if(file->references<=0) {
		lock_release(file->flock);
		vfs_close(file->v_ptr);
		lock_destroy(file->flock);
		kfree(file);
	}
	else{
		lock_release(file->flock);
	}
	open_file_cnt--;
	return 0;
}


int sys_read(int filehandler, userptr_t buf, size_t size, int *ret) {
	struct iovec iov;
	struct uio myuio;
	if(filehandler < 0 || filehandler >= MAX_PROCESS_OPEN_FILES || !curproc->file_table[filehandler]) {
		return EBADF;
	}
	struct File *file = curproc->file_table[filehandler];

	int how = file->open_flags & O_ACCMODE;
	if (how == O_WRONLY) {
		return EBADF;
	}
	lock_acquire(file->flock);
	off_t old_offset = file->offset;

	uio_uinit(&iov, &myuio, buf, size, file->offset, UIO_READ);

	int result = VOP_READ(file->v_ptr, &myuio);
	if (result) {
		lock_release(file->flock);
		return result;
	}

	file->offset = myuio.uio_offset;
	*ret = file->offset - old_offset;
	lock_release(file->flock);
	return 0;
}

int write_util(int filehandler,int max_size)
{
	int i,j=0;
	int max=3;
	for(i=0;i<filehandler;i++)
	{
		max++;
		if(filehandler==0){
			j++;
			max_size=max_size-1;
		}
	}
	return 0;
}

int sys_write(int filehandler, userptr_t buf, size_t size, int *ret) {
	struct iovec fiovec;
	struct uio fuio;
	int io=0;
	if(filehandler < 0 || filehandler >= MAX_PROCESS_OPEN_FILES || !curproc->file_table[filehandler]) {		
		return EBADF;
	}
	struct File *file = curproc->file_table[filehandler];
	int how = file->open_flags & O_ACCMODE;
	if (how == O_RDONLY) {
		return EBADF;
	}
	io++;
	io=write_util(filehandler,2);
	lock_acquire(file->flock);
	off_t old_offset = file->offset;
	uio_uinit(&fiovec, &fuio, buf, size, file->offset, UIO_WRITE);
	int result = VOP_WRITE(file->v_ptr, &fuio);
	if (result) {
		lock_release(file->flock);
		return result;
	}
	file->offset = fuio.uio_offset;
	*ret = file->offset - old_offset;
	lock_release(file->flock);
	return 0;
}

int dup2_helper(int oldfd,int newfd,int cnt)
{
		if(oldfd < 0 || oldfd >= MAX_PROCESS_OPEN_FILES || newfd < 0 || newfd >= MAX_PROCESS_OPEN_FILES || !curproc->file_table[oldfd]) {
		return EBADF;
	}
	if(oldfd == newfd){
		return 0;
	}
	if(curproc->file_table[newfd]){
		int result = sys_close(newfd);
		if(result){
			return result;
		}
	}
	cnt++;
	struct File *file = curproc->file_table[newfd] = curproc->file_table[oldfd];
	cnt--;
	lock_acquire(file->flock);
	file->references++;
	lock_release(file->flock);
	return 0;
}

int sys_dup2(int oldfd, int newfd) {
	int cnt=0;
	return dup2_helper(oldfd,newfd,cnt);
}

int seek_helper(int seek_cr){
	int i;
	int max=3;
	for(i=0;i<seek_cr;i++)
	{
		max++;
	}
	return 0;
}
int sys_lseek(int fd, off_t pos, userptr_t whence_ptr, off_t *ret) {
	struct File *file;
	struct stat stats;
	int whence,seek_cr;
	if(fd < 0 || fd > MAX_PROCESS_OPEN_FILES || !(file = curproc->file_table[fd])){
		return EBADF;
	}
	seek_cr++;
	if(!VOP_ISSEEKABLE(file->v_ptr)){
		return ESPIPE;
	}
	int result = VOP_STAT(file->v_ptr, &stats);
	if(result){
		return result;
	}
	seek_cr--;
	result = copyin(whence_ptr, &whence, sizeof(int));
	if(result) {
		return result;
	}

	seek_cr=seek_helper(seek_cr);

	if(whence==SEEK_SET){
					if(pos < 0){
				return EINVAL;
			}
			lock_acquire(file->flock);
			*ret = file->offset = pos;
			lock_release(file->flock);
	}
	else if(whence==SEEK_CUR){
					lock_acquire(file->flock);
			if(file->offset + pos < 0){
				lock_release(file->flock);
				return EINVAL;
			}
			*ret = file->offset += pos;
			lock_release(file->flock);
	}
	else if(whence==SEEK_END){
					if(stats.st_size + pos < 0){
				return EINVAL;
			}
			lock_acquire(file->flock);
			*ret = file->offset = stats.st_size + pos; 
			lock_release(file->flock);
	}
	else{
		return EINVAL;
	}
	
	return 0;
}
