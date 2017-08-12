/* Copyright 2014, ACSE & CADIEEL
 *    ACSE   : http://www.sase.com.ar/asociacion-civil-sistemas-embebidos/ciaa/
 *    CADIEEL: http://www.cadieel.org.ar
 * All rights reserved.
 *
 * This file is part of CIAA Firmware.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/** \brief Short description of this file
 **
 ** Contains the file system POSIX API, vfs implementation and file descriptor (file objects) implementation.
 ** The POSIX API contains common functions as open(), close(), read(), write().
 ** The VFS is an abstraction layer over the many file system implementations.
 ** File objects are structures that relate an open file with a process in execution.
 ** File objects have fields such as corresponding node and current position in file.
 **/

/** \addtogroup CIAA_Firmware CIAA Firmware
 ** @{ */
/** \addtogroup Template Template to start a new module FIXME
 ** @{ */

/*
 * Initials     Name
 * ---------------------------
 * MZ           Marcos Ziegler
 */

/*
 * modification history (new versions first)
 * -----------------------------------------------------------
 * 20160101 v0.0.1 MZ initial version
 */

/*
TODO:
-Use only static memory
-Multitasking support: Mutex, semaphores and queues
-Implement a default root filesystem, to create files in ram
*/

/*
FIXME:
-Modify blockdevice driver. Create all device vnodes in directory /dev. /dev is then a mountpoint
*/

/*==================[inclusions]=============================================*/
#include <string.h>
#include <stdbool.h>
#include "vfs.h"
#include "tlsf.h"
#include "ooc.h"
#include "device.h"

/*==================[macros and definitions]=================================*/

#define ASSERT_MSG(cond, msg) assert_msg((cond), (msg), __FILE__, __LINE__)

/* Define 8K arena for FS */
#define FS_ARENA_MEMORY (8 * 1024)

/*==================[internal data declaration]==============================*/

/*==================[internal functions declaration]=========================*/

#if 0
/** \brief Auxiliary recursive function of vfs_get_mntpt_path()
 **
 **
 **
 ** \param[in] node node whose path is to be found
 ** \param[out] path path string returned
 ** \param[out] len_p pointer to the path size variable
 ** \return -1 if an error occurs, in other case 0
 **/
static int vfs_get_mntpt_path_rec(vnode_t *node, char *path, uint16_t *len_p);

/** \brief get the path relative to the mountpoint
 **
 **
 **
 ** \param[in] node node whose mount point path is to be found
 ** \param[out] path string which will contain the mountpoint path
 ** \param[out] len_p size of path string
 ** \return -1 if an error occurs, in other case 0
 **/
static int vfs_get_mntpt_path(vnode_t *node, char *path, uint16_t *len_p);
#endif

/** \brief alloc memory for new node and initialize contents
 **
 **
 **
 ** \param[in] name
 ** \param[in] name_len
 ** \return created node if success, else NULL
 **/
static vnode_t *vfs_node_alloc(const char *name, size_t name_len);

/** \brief create subtree given a path
 **
 ** Creates intermediate directories
 ** TODO: Implement tree creation from relative path too
 ** Path must contain initial '/', indicating its a root path
 ** Created leaf will be VFS_FTREG type
 **
 ** \param[in] path
 ** \param[out] inode_p
 ** \return -1 if the directory already exists FIXME
 **/
static int vfs_inode_reserve(const char *path, vnode_t **inode_p);

/** \brief search inode by path
 **
 ** postconditions:
 **    path_p points to the beginning first non-found element in path
 **    ret_node_p points to the last found node in the path
 **
 ** \param[in] path_p pointer to the path string to be searched
 ** \param[in] ret_node_p pointer to the found node address
 ** \return -1 if an error occurs, in other case 0
 **/
static int vfs_inode_search(char **path_p, vnode_t **ret_node_p);

/** \brief get a driver of a specified filesystem by name
 **
 **
 **
 ** \param[in] driver_name name of the driver as given in its corresponding filesystem_driver struct
 ** \return pointer to searched fs driver if success, NULL otherwise
 **/
static filesystem_driver_t *vfs_get_driver(const char *driver_name);

/** \brief initialize file descriptor table
 **
 ** alloc memory, initialize values, etc.
 **
 ** \return -1 if an error occurs, in other case 0
 **/
static int file_descriptor_table_init(void);

/** \brief
 **
 **
 **
 ** \param[in] node the new file descriptor will be linked to this file
 ** \return pointer to created descriptor structure if success, else NULL
 **/
static file_desc_t *file_desc_create(vnode_t *node);

/** \brief free a file descriptor
 **
 ** post-condition: Set pointer to NULL
 **
 ** \param[in] file_desc descriptor to be freed
 ** \return -1 if an error occurs, in other case 0
 **/
static int file_desc_destroy(file_desc_t *file_desc);

#if 0
/** \brief get a descriptor structure by its number
 **
 **
 **
 ** \param[in] index requested descriptor structure modules/posix/test/mtest/test_fs/src/vfs.c:230:1: warning: initialization from incompatible pointer typeindex
 ** \return -1 if an error occurs, in other case 0
 **/
static file_desc_t *file_desc_get(uint16_t index);
#endif

/*==================[internal data definition]===============================*/

/** \brief Root inode
 *
 * Root inode of the entire vfs
 *
 */
static vnode_t *vfs_root_inode = NULL;

/** \brief Filesystem drivers declaration
 *
 * Here are the drivers defined in the lower layer file system implementations (in ext2.c, fat.c, etc.)
 *
 */
extern filesystem_driver_t ext2_driver;
extern filesystem_driver_t pseudofs_driver;
extern filesystem_driver_t blockdev_driver;

/** \brief Filesystem drivers table
 *
 * Driver table used by vfs_get_driver() to retrieve requested driver
 *
 */
static filesystem_driver_t *vfs_fsdriver_table[] =
{
   &ext2_driver,
   &blockdev_driver,
   &pseudofs_driver,
   NULL,
   NULL
};

/** \brief File descriptors table
 *
 * TODO: Every thread should have its own table
 *
 */
static file_descriptor_table_t _f_tab;
static file_descriptor_table_t *file_desc_tab_p = &_f_tab;

/** \brief tlsf dynamic memory pool. Use uint32_t to align to 32 bits
 *
 */
uint32_t fs_arena_area[FS_ARENA_MEMORY/sizeof(uint32_t)];

/*==================[external data definition]===============================*/
/** \brief memory pool handle
 *
 */
tlsf_t fs_mem_handle;

/*==================[internal functions definition]==========================*/

static void assert_msg(int cond, char* msg, char * file, int line)
{
   if (cond)
   {
      /* assertion is ok */
      //printf("OK: Assert in %s:%d\n", file, line);
   }
   else
   {
      //printf("ERROR: Assert Failed in %s:%d %s\n", file, line, msg);
   }
}

#if 0
static int vfs_get_mntpt_path_rec(vnode_t *node, char *path, uint16_t *len_p)
{
   uint16_t name_len;
   int ret;
   if(false == node->f_info.is_mount_dir)
   {
      ret = vfs_get_mntpt_path_rec(node->parent_node, path, len_p);
      if(ret)
      {
         return -1;
      }
   }
   name_len = strlen(node->f_info.file_name);
   if(*len_p + name_len > FS_PATH_MAX-1)
      return -1;
   strcpy((char *)path+(*len_p), node->f_info.file_name);
   *len_p += strlen(node->f_info.file_name);
   return 0;
}

static int vfs_get_mntpt_path(vnode_t *node, char *path, uint16_t *len_p)
{
   int ret;
   *len_p = 0;
   ret = vfs_get_mntpt_path_rec(node, path, len_p);
   return ret;
}
#endif

static vnode_t *vfs_node_alloc(const char *name, size_t name_len)
{
   vnode_t * new_node;

   ASSERT_MSG(name_len, "\tvfs_node_alloc(): !name_len failed");
   if (!name_len)
   {
      return NULL;
   }
   ASSERT_MSG((*name) && (name_len <= FS_NAME_MAX), "\tvfs_node_alloc(): !(*name) || name_len > FS_NAME_MAX failed");
   if (!(*name) || name_len > FS_NAME_MAX)
   {
      return NULL;
   }

   new_node = (vnode_t *) tlsf_malloc(fs_mem_handle, sizeof(vnode_t));
   ASSERT_MSG(NULL != new_node, "\tvfs_node_alloc(): !(*name) || name_len > FS_NAME_MAX failed");
   if (NULL == new_node)
   {
      return NULL;
   }
   //tlsf_debug_print(fs_mem_handle);
   memset(new_node, 0, sizeof(vnode_t));
   strncpy(new_node->f_info.file_name, name, name_len);
   new_node->f_info.file_name[name_len] = '\0';
   new_node->f_info.file_namlen = name_len;

   return new_node;
}

extern int vfs_node_free(vnode_t *node)
{
   if(NULL != node)
   {
      tlsf_free(fs_mem_handle, (void *)node);
   }
   return 0;
}

/* TODO: If error, eliminate the subtree */
static int vfs_inode_reserve(const char *path, vnode_t **inode_p)
{
   int ret;
   vnode_t *inode, *child;
   char *p_start, *p_end, *aux_path;

   *inode_p = NULL;
   aux_path = (char *)path;
   ret = vfs_inode_search(&aux_path, &inode);
   ASSERT_MSG(0 > ret, "\tvfs_inode_reserve(): Node already exists");
   if(0 <= ret)
   {
      /* node already exists */
      return -1;
   }
   /* Now inode points to the last found node in path, por lo que hay que empezar creando inode->child
    * inode->child_node will be the first non existing element in the new subtree
    * aux_path points to the first character of the first non existent element in path. So inode->child_node
    * should be named as this element
    */
   p_start = p_end = aux_path;
   for(p_start = aux_path; *p_start == '/'; p_start++);   /* skip initial '/'s */
   for(p_end=p_start; *p_end!='\0' && *p_end!='/'; p_end++);
   /* Now p_start and p_end point to the next element in path string.
    * p_start points to the first char of the first non existent element in path,
    * p_end points to the end of the string '\0' if the next element is the last element in the path.
    * Otherwise, p_end points to the '/' character that indicates the end of the name of this element
    * and the start of the next. This indicates that this is not the last element.
    * "node" should point to the new node
    */
   ASSERT_MSG(p_end != p_start, "\tvfs_inode_reserve(): Invalid path");
   if(p_end == p_start)
   {
      /* This is true if the path had only '/' elements. Return error */
      return -1;
   }
   ASSERT_MSG(*p_end != '/', "\tvfs_inode_reserve(): Invalid path. Path finishes with /");
   if(*p_end == '/')
   {
      /* This is true if there are multiple non existent elements in the path.
       * Return error to simplify implementation.
       */
      return -1;
   }
   child = vfs_create_child(inode, p_start, (uint32_t)(p_end-p_start), 0);
   *inode_p = child;
   ASSERT_MSG(NULL != child, "\tvfs_inode_reserve(): vfs_create_child() failed");
   if(NULL == child)
   {
      return -1;
   }
   /* Created node is directory by default */
   child->f_info.type = VFS_FTDIR;

   return 0;
}

static int vfs_inode_search(char **path_p, vnode_t **ret_node_p)
{
   uint16_t pnamelen; /* Path element namelength */
   char *p_start, *p_end;
   uint32_t ret;
   vnode_t *node;

   if (NULL == *path_p || (*path_p)[0] == '\0' || *(path_p)[0] != '/')
   {
      *ret_node_p = NULL;
      return -1;
   }

   *ret_node_p = node = vfs_root_inode;

   p_start = p_end = *path_p;
   /* When the loop finishes, p_start points to the first characer after '/' */
   for(;*p_start=='/';p_start++);
   *path_p = p_start;
   if('\0' == *p_start)   /* Path only has '/'. Return root inode */
   {
      *ret_node_p=vfs_root_inode;
      return 0;
   }
   for(p_end=p_start; '\0' != *p_end && '/' != *p_end; p_end++);
   node = node->child_node;   /* Start at the first level after root */
   /* Loop: If node is null in this iteration, it means that in this level, none of the siblings
    * matches the path element. The path is then not valid.
    */
   while(NULL != node)
   {
      /* p_start and p_end point to the beginning and end of the element in the path string to be
       * processed in this iteration.
       * p_start points to the first character of the element.
       * p_end points to the character next to the last character of the element
       */
      pnamelen = p_end-p_start;   /* Length of the name between '/' not counting '\0' */
      while(NULL != node)
      {
         ret = strncmp(p_start, node->f_info.file_name, pnamelen);
         if(!ret)
         {
            /* Names match. Must analyze the child node */
            break;
         }
      /* node name does not match with the path element. Try with the next node in the current level */
         node = node->sibling_node;
      }
      if(NULL == node)
      {
         /* Didnt find a node in this level whose name matches the path element searched.
          * So this is an invalid path
          * ret_node_p will point to the last matching node found
          */
         return -1;
      }
      /* The path element matches with the name of this node.
       * Iterate again in the next element of the tree to find the node matching the next element in path
       *
       */
      *ret_node_p = node;
      node = node->child_node;
      p_start = p_end;
      /* When the loop finishes, p_start points to the first char after '/' */
      for(; '/' == *p_start; p_start++);
      *path_p = p_start;
      if('\0' == *p_start)   /* The end of the searched path has been reached */
      {
         /* Last node found is the one that must be returned. Success */
         return 0;
      }
      /* When this loop finishes, p_end points to the '/' after the last path element found.
       * If the last path element found is the last elment of the searched path,
       * p_end will point to the '\0' character of the path string.
       */
      for(p_end = p_start; '\0' != *p_end && '/' != *p_end; p_end++);
   }
   /* The path contains more elements than the tree depth.
    * Searched node doesnt exist. Failure
    */
   return -1;
}

static filesystem_driver_t *vfs_get_driver(const char *driver_name)
{
   int i;

   for(i=0; NULL != vfs_fsdriver_table[i]; i++)
   {
      if(!strcmp(driver_name, vfs_fsdriver_table[i]->driver_name))
         return vfs_fsdriver_table[i];
   }
   return NULL;
}

/*#######################FILE_DESC######################*/

static int file_descriptor_table_init(void)
{
   memset(file_desc_tab_p, 0, sizeof(file_descriptor_table_t));
   return 0;
}

static file_desc_t *file_desc_create(vnode_t *node)
{
   file_desc_t *file;
   uint16_t i;

   if(file_desc_tab_p->n_busy_desc >= VFS_DESC_MAX)
      return NULL;

   for(i=0; i<VFS_DESC_MAX; i++)
   {
      if(NULL == file_desc_tab_p->table[i])
         break;
   }

   /* Ahora i es el indice del primer file_desc desocupado en la tabla */
   if(VFS_DESC_MAX == i)   /* No hay file_desc libres */
      return NULL;
   /* Allocate new descriptor */
   file = (file_desc_t *) tlsf_malloc(fs_mem_handle, sizeof(file_desc_t));
   if(NULL == file)
      return NULL;
   memset(file, 0, sizeof(file_desc_t));
   file->node = node;

   file_desc_tab_p->table[i] = file;
   file_desc_tab_p->n_busy_desc++;

   file->node = node;
   file->index = i;

   return file;
}

static int file_desc_destroy(file_desc_t *file_desc)
{
   uint16_t index;

   index = file_desc->index;
   tlsf_free(fs_mem_handle, (void *)file_desc_tab_p->table[index]);
   file_desc_tab_p->table[index] = NULL;
   if(file_desc_tab_p->n_busy_desc)
      file_desc_tab_p->n_busy_desc--;
   return 0;
}

#if 0
file_desc_t *file_desc_get(uint16_t index)
{
   if(index >= VFS_DESC_MAX)
      return NULL;

   return file_desc_tab_p->table[index];
}
#endif


/*########################/FILE_DESC#####################*/

/*########################AUXILIAR#####################*/
static int vfs_print_tree_rec(vnode_t *node, int level)
{
   int ret, i;

   if(NULL == node)
   {
      return 0;
   }
   for(i=0; i<level; i++)
      //printf("---");
   //printf("%s: address: %p child: %p \n",node->f_info.file_name, node, node->child_node);

   ret=vfs_print_tree_rec(node->child_node, level+1);
   if(ret)
   {
      return -1;
   }
   ret=vfs_print_tree_rec(node->sibling_node, level);
   if(ret)
   {
      return -1;
   }

   return 0;
}

int vfs_print_tree(void)
{
   return vfs_print_tree_rec(vfs_root_inode, 0);
}
/*########################/AUXILIAR#####################*/

/*==================[external functions definition]==========================*/

extern vnode_t *vfs_create_child(vnode_t *parent, const char *name, size_t namelen, mode_t mode)
{
   vnode_t *child;

   child = vfs_node_alloc(name, namelen);
   ASSERT_MSG(NULL != child, "\tvfs_create_child(): vfs_node_alloc() failed");
   if(NULL == child)
      return NULL;
   child->sibling_node = parent->child_node;
   parent->child_node = child;
   child->parent_node = parent;
   /* Copy parents fs info. Child belongs to same mountpoint */
   child->fs_info = parent->fs_info;
   child->f_info.file_name[namelen] = '\0';
   child->f_info.down_layer_info = NULL;
   return child;
}

extern int vfs_delete_child(vnode_t *child)
{
   vnode_t *node_p, *auxnode_p;
   int ret;
   /* child->sibling_node == NULL: Single child
    * child->parent_node == NULL: Root inode
    * Must delete child. Special case if child to delete is head child.
    * General case:
    *
    *
    */
   ASSERT_MSG(NULL != child, "vfs_delete_child(): Node doesnt exist");
   if(NULL == child)
   {
      /* Node does not exist. Error */
      return -1;
   }
   ASSERT_MSG(NULL != child->parent_node, "vfs_delete_child(): Cant delete root inode");
   if(NULL == child->parent_node)
   {
      /* Root inode. Cant delete it */
      return -1;
   }
   /* Find previous sibling node (not a double-linked list :( ) */
   node_p = child->parent_node->child_node;   /* Head of children list */
   ASSERT_MSG(NULL != node_p, "vfs_delete_child(): Parent dont match child tree");
   if(NULL == node_p)
   {
      return -1;
   }
   auxnode_p = NULL;
   /* node_p points to the head of the children list */
   while(node_p != child && NULL != node_p)
   {
      auxnode_p = node_p;
      node_p = node_p->sibling_node;
   }
   /* now auxnode_p points to the previous sibling of node_p */
   ASSERT_MSG(NULL != node_p, "vfs_delete_child(): Parents dont match");
   if(NULL == node_p)
   {
      /* Could not find this node in the list of children of its own parent. Parents dont match, should never happen.
       * System consistency error.
       */
      return -1;
   }
   if(auxnode_p == NULL)
      /* Free head child */
      child->parent_node->child_node = node_p->sibling_node;
   else
      auxnode_p->sibling_node = node_p->sibling_node;
   ret = vfs_node_free(child);
   ASSERT_MSG(-1 < ret, "vfs_delete_child(): Couldnt free node");
   if(ret)
   {
      return -1;
   }

   return 0;
}

extern int vfs_init(void)
{
   int ret;

   memset(fs_arena_area, 0, sizeof(fs_arena_area));
   fs_mem_handle = tlsf_create_with_pool((void *)fs_arena_area, sizeof(fs_arena_area));
   ASSERT_MSG(0 != fs_mem_handle, "vfs_init(): Could not init memory pool");
   if (0 == fs_mem_handle)
   {
      return -1;
   }

   /* Create root node */
   vfs_root_inode = (vnode_t *) tlsf_malloc(fs_mem_handle, sizeof(vnode_t));
   ASSERT_MSG(NULL != vfs_root_inode, "vfs_init(): mem error");
   if(NULL == vfs_root_inode)
   {
      return -1;
   }
   //tlsf_debug_print(fs_mem_handle);
   vfs_root_inode->parent_node = NULL;
   vfs_root_inode->sibling_node = NULL;
   vfs_root_inode->f_info.is_mount_dir = true;
   vfs_root_inode->f_info.type = VFS_FTDIR;
   vfs_root_inode->f_info.file_name[0] = '\0'; /* Special name for root */
   /* Reserve mem for root mountpoint information */
   vfs_root_inode->fs_info = (filesystem_info_t *) tlsf_malloc(fs_mem_handle, sizeof(filesystem_info_t));
   ASSERT_MSG(NULL != vfs_root_inode->fs_info, "vfs_init(): Could not create mountpoint fsinfo");
   if (NULL == vfs_root_inode->fs_info)
   {
      return -1;
   }
   //tlsf_debug_print(fs_mem_handle);
   /* Driver for simple ram fs */
   vfs_root_inode->fs_info->drv = vfs_get_driver("pseudofs");
   ASSERT_MSG(NULL != vfs_root_inode->fs_info->drv, "vfs_init(): vfs_get_driver() failed");
   if(NULL == vfs_root_inode->fs_info->drv)
   {
      return -1;
   }

   /* Initialize file descriptors */
   ret = file_descriptor_table_init();
   ASSERT_MSG(-1 != ret, "vfs_init(): file_descriptor_table_init() failed");
   if(ret)
   {
      return -1;
   }

   return 0;
}

/** \brief Format a device
 *
 *
 *
 */
extern int vfs_format(filesystem_info_t **fs, void *param)
{
   int ret;

   /* Llamo a la funcion de bajo nivel */
   ret = (*fs)->drv->driver_op->fs_format(*fs, param);
   ASSERT_MSG(0 == ret, "format(): Lower layer format failed");
   if(ret)
   {
      /* Fallo el format de bajo nivel */
      return -1;
   }
   return 0;
}

/** \brief VFS mount
 *
 * Ir a la definicion de POSIX
 *
 */
extern int vfs_mount(char *target_path, filesystem_info_t **fs)
{
   char *tpath;
   int ret;
   vnode_t *targetnode;

   tpath = target_path;
   /* Reserve vnode for the mountpoint */
   ret = vfs_inode_reserve(tpath, &targetnode);
   ASSERT_MSG(0 == ret, "mount(): vfs_inode_reserve() failed creating target node");
   if(ret)
   {
      /* The directory already exists */
      /* FIXME: It should not be needed that the dir doesnt exist.
       * Should overwrite new mount in node
       */
      return -1;
   }
   /* Set filesystem information in new node */
   targetnode->fs_info = *fs;
   ASSERT_MSG(NULL != targetnode->fs_info, "vfs_mount(): Could not create mountpoint fsinfo");
   if (NULL == targetnode->fs_info)
   {
      return -1;
   }

   targetnode->f_info.is_mount_dir = true;
   /* Call the lower layer method */
   ret = (*fs)->drv->driver_op->fs_mount(*fs, targetnode);
   ASSERT_MSG(0 == ret, "mount(): Lower layer mount failed");
   if(ret)
   {
      /* Lower layer mount failed */
      return -1;
   }
   return 0;
}

/* Cant support over mounting. Directory must be empty to mount.
 * Umount eliminates mount root node
 */
/* TODO: Check if exists an open file who belongs to this mount */
extern int vfs_umount(const char *target_path)
{
   char *tpath;
   filesystem_driver_t *fs_driver;
   int ret;
   vnode_t *targetnode;

   tpath = (char *)target_path;
   ret = vfs_inode_search(&tpath, &targetnode);
   ASSERT_MSG(0 == ret, "umount(): vfs_inode_search() failed. Mountpoint doesnt exist");
   if(ret)
   {
      /* The device node doesnt exist */
      return -1;
   }
   ASSERT_MSG(true == targetnode->f_info.is_mount_dir, "umount(): Target not a mountpoint");
   if(true != targetnode->f_info.is_mount_dir)
   {
      /* Not a mountpoint */
      return -1;
   }
   /* Files under this mount still in use. Close them before umount */
   ASSERT_MSG(0 == targetnode->fs_info->ref_count, "umount(): Mountpoint still in use, cant umount");
   if(0 != targetnode->fs_info->ref_count)
   {
      return -1;
   }
   /* Call the lower layer method */
   fs_driver = targetnode->fs_info->drv;
   if(NULL == fs_driver->driver_op->fs_umount)
   {
      /* method not supported */
      return -1;
   }
   ret = fs_driver->driver_op->fs_umount(targetnode);
   ASSERT_MSG(0 == ret, "mount(): Lower layer mount failed");
   if(ret)
   {
      /* Lower layer mount failed */
      return -1;
   }

   /* Delete mountpoint vnode */
   ret = vfs_delete_child(targetnode);
   ASSERT_MSG(0 == ret, "vfs_umount(): vfs_delete_child() failed");
   if(ret)
   {
      return -1;
   }

   return 0;
}

/*
 *   TODO: Validate path
 */
extern int vfs_mkdir(const char *dir_path, mode_t mode)
{
   vnode_t *dir_inode_p;
   int               ret;

   char *tpath = (char *) dir_path;
   /* Create an empty node in dir_path */
   /* vfs_inode_reserve(): The new node inherits the fathers fs info */
   ret = vfs_inode_reserve(tpath, &dir_inode_p);
   if(0 > ret)
   {
      /* Directory already exists or path is invalid */
      return -1;
   }
   /* Fill the vnode fields */
   dir_inode_p->f_info.type = VFS_FTDIR;
   ret = dir_inode_p->fs_info->drv->driver_op->fs_create_node(dir_inode_p->parent_node, dir_inode_p);
   if(ret)
   {
      /* Could not create lower layer node. Handle the issue */
      return -1;
   }

   return 0;
}

/* FIXME: Check if it is a directory before deleting */
/* FIXME: Check if dir has children before deleting */
extern int vfs_rmdir(const char *dir_path)
{
   vnode_t *target_inode;
   filesystem_driver_t *driver;
   int               ret;
   char *auxpath;

   auxpath = (char *) dir_path;
   ret = vfs_inode_search(&auxpath, &target_inode);   /* Return 0 if node found */
   ASSERT_MSG(0 == ret, "vfs_rmdir(): vfs_inode_search() failed. Device doesnt exist");
   if(ret)
   {
      /* The directory doesnt exist */
      return -1;
   }
   /* Check if this is the root inode */
   if(NULL == target_inode->parent_node)
   {
      /* Cannot erase root */
      return -1;
   }
   /* Check if this iss a directory */
   ASSERT_MSG(VFS_FTDIR == target_inode->f_info.type, "vfs_rmdir(): not a directory file");
   if(VFS_FTDIR != target_inode->f_info.type)
   {
      /* Not a regular file, can not unlink */
      return -1;
   }
   /* Check if this directory still has children */
   ASSERT_MSG(NULL == target_inode->child_node, "vfs_rmdir(): Directory still has children. Cant delete");
   if(NULL != target_inode->child_node)
   {
      /* Cant delete directory with children */
      return -1;
   }
   /* Check FS driver */
   driver = target_inode->fs_info->drv;
   ASSERT_MSG(NULL != driver, "vfs_rmdir(): driver not available");
   if(NULL == driver)
   {
      /*No driver. Fatal error*/
      return -1;
   }
   /* Check driver operation */
   ASSERT_MSG(NULL != driver->driver_op->fs_delete_node, "vfs_rmdir(): delete op not available");
   if(NULL == driver->driver_op->fs_delete_node)
   {
      /*The filesystem driver does not support this method*/
      return -1;
   }
   /* Call low layer delete function */
   ret = driver->driver_op->fs_delete_node(target_inode->parent_node, target_inode);
   ASSERT_MSG(0 == ret, "vfs_rmdir(): lower layer unlink failed");
   if(ret)
   {
      /* Filesystem op failed */
      return -1;
   }
   /* TODO: Remove node from vfs */
   ret = vfs_delete_child(target_inode);
   ASSERT_MSG(0 == ret, "vfs_rmdir(): vfs_delete_child() failed");
   if(ret)
   {
      /* Filesystem op failed */
      return -1;
   }

   return 0;
}

/*
 *   TODO: Current implementation can create a subtree. Should only create a leaf. Validate path.
 */
extern int vfs_open(const char *path, file_desc_t **file, int flags)
{
   vnode_t *target_inode, *parent_inode;
   char *auxpath;
   int ret;

   auxpath = (char *) path;
   ret = vfs_inode_search(&auxpath, &target_inode);   /* Return 0 if node found */
   if (ret)
   {
      /* Node does not exist. Must create a new file if VFS_O_CREAT set */
      if (flags & VFS_O_CREAT)
      {
         /* Create empty node in dir_path */
         /* vfs_inode_reserve(): The new node inherits the fathers fs info */
         auxpath= (char *)path;
         ret = vfs_inode_reserve(auxpath, &target_inode);
         ASSERT_MSG(NULL != target_inode, "open(): vfs_inode_reserve() failed");
         if(NULL == target_inode)
         {
            /* Error when allocating node */
            return -1;
         }
         target_inode->f_info.type = VFS_FTREG;
         /* Create node in lower layer */
         parent_inode = target_inode->parent_node;
         ret = target_inode->fs_info->drv->driver_op->fs_create_node(parent_inode, target_inode);
         ASSERT_MSG(ret>=0, "open(): fs_create_node() failed");
         if(ret)
         {
            /* Could not create lower layer node. Handle the issue */
            return -1;
         }
      }
      else
      {
         /* Node does not exist, but VFS_O_CREAT not set. Error */
         return -1;
      }
   }
   else      /* El nodo ya existe, no hay que crearlo, solo abrir el archivo */
   {
      /* Node exists. Only open the file.
       * If O_CREAT was set, if the file already exists it is an error.
       */
      if ((flags & VFS_O_EXCL) && (flags & VFS_O_CREAT))
      {
         return -1;
      }
      /* TODO: Contemplate more cases of filetypes */
      if(VFS_FTDIR == target_inode->f_info.type || VFS_FTUNKNOWN == target_inode->f_info.type)
      {
         return -1;
      }
   }

   *file = file_desc_create(target_inode);
   ASSERT_MSG(NULL != *file, "open(): file_desc_create() failed");
   if(NULL == *file)
   {
      return -1;
   }
   ret = (*file)->node->fs_info->drv->driver_op->file_open(*file);
   ASSERT_MSG(ret >= 0, "open(): file_open() failed");
   if(ret)
   {
      file_desc_destroy(*file);
      *file = NULL;
      return -1;
   }

   //printf("vfs_open(): filename: %.*s refcount: %d\n", (*file)->node->f_info.file_namlen,
                     //(*file)->node->f_info.file_name, (*file)->node->f_info.ref_count);
   (*file)->node->fs_info->ref_count++;
   (*file)->node->f_info.ref_count++;
   return 0;
}

extern int vfs_close(file_desc_t **file)
{
   vnode_t *node;
   ssize_t ret;

   /* Assert the file descriptor */
   ASSERT_MSG(NULL != *file, "vfs_close(): invalid file_desc_get()");
   if(NULL == *file)
   {
      /* Invalid file descriptor */
      return -1;
   }

   node = (*file)->node;
   ret = file_desc_destroy(*file); //Aca hay un problema. FIXME. Cambia la info del nodo
   ASSERT_MSG(0 == ret, "vfs_close(): file_desc_destroy() failed");
   if(ret)
   {
      return -1;
   }
   *file = NULL;

   node->fs_info->ref_count--;
   node->f_info.ref_count--;

   return 0;
}

extern ssize_t vfs_read(file_desc_t **file, void *buf, size_t nbytes)
{
   ssize_t ret;

   /* Assert the file object */
   ASSERT_MSG(NULL != *file, "read(): file object invalid");
   if(NULL == *file)
   {
      /* Invalid file descriptor */
      return -1;
   }

   /* Verify that the lower layer driver implements the read operation */
   ASSERT_MSG(NULL != (*file)->node->fs_info->drv->driver_op->file_read, "read(): read op not available");
   if(NULL == (*file)->node->fs_info->drv->driver_op->file_read)
   {
      /* Driver doesnt support read */
      return 1;
   }
   /* Lower layer read operation */
   ret = (*file)->node->fs_info->drv->driver_op->file_read((*file), buf, nbytes);
   ASSERT_MSG(ret == nbytes, "read(): file_read() failed");
   if(ret != nbytes)
   {
      //printf("vfs_read(): Expected to read %d, readed %d\n", nbytes, ret);
      /* TODO: Set ERROR to show that could not read nbytes */
   }
   return ret;
}

extern ssize_t vfs_write(file_desc_t **file, void *buf, size_t nbytes)
{
   ssize_t ret;

   /* Assert the file object */
   ASSERT_MSG(NULL != *file, "read(): file object invalid");
   if(NULL == *file)
   {
      /* Invalid file descriptor */
      return -1;
   }

   /* Verify that the lower layer driver implements the write operation */
   if(NULL == (*file)->node->fs_info->drv->driver_op->file_write)
   {
      /* Driver doesnt support write */
      return 1;
   }
   /* Lower layer write operation */
   ret = (*file)->node->fs_info->drv->driver_op->file_write((*file), buf, nbytes);
   ASSERT_MSG(ret == nbytes, "write(): file_write() failed");
   if(ret!=nbytes)
   {
      /* TODO: Set ERROR to show that could not write nbytes */
      return ret;
   }
   return ret;
}

extern ssize_t vfs_lseek(file_desc_t **file, ssize_t offset, int whence)
{
   ssize_t pos;

   /* Assert file object */
   ASSERT_MSG(NULL != *file, "read(): file_desc_get() failed");
   if(NULL == *file)
   {
      /* Invalid file descriptor */
      return -1;
   }

   pos=0;
   switch(whence)
   {
      case SEEK_END:
         pos = (*file)->cursor + offset;
         break;
      case SEEK_CUR:
         pos = (*file)->cursor + offset;
         break;
      default:
         pos = offset;
         break;
   }

   if ((pos >= 0) && (pos <= (*file)->node->f_info.file_size))
   {
      (*file)->cursor = (uint32_t) pos;
   }

   return (*file)->cursor;
}

extern int vfs_unlink(const char *path)
{
   vnode_t *target_inode;
   filesystem_driver_t *driver;
   char *auxpath;
   int ret;

   auxpath = (char *) path;
   ret = vfs_inode_search(&auxpath, &target_inode);   /* Return 0 if node found */
   ASSERT_MSG(0 == ret, "unlink(): vfs_inode_search() failed");
   if(ret)
   {
      /* File not found */
      return -1;
   }
   ASSERT_MSG(VFS_FTREG == target_inode->f_info.type, "unlink(): not a regular file");
   if(VFS_FTREG != target_inode->f_info.type)
   {
      /* Not a regular file, can not unlink */
      return -1;
   }
   //printf("unlink(): file_name: %.*s refcount: %d\n", target_inode->f_info.file_namlen,
                     //target_inode->f_info.file_name, target_inode->f_info.ref_count);
   ASSERT_MSG(0 == target_inode->f_info.ref_count, "unlink(): Node still in use. Cant unlink");
   if(0 != target_inode->f_info.ref_count)
   {
      /* Node still in use. Dont delete */
      return -1;
   }
   driver = target_inode->fs_info->drv;
   ASSERT_MSG(NULL != driver, "unlink(): driver not available");
   if(NULL == driver)
   {
      /*No driver. Fatal error*/
      return -1;
   }
   ASSERT_MSG(NULL != driver->driver_op->fs_delete_node, "unlink(): delete op not available");
   if(NULL == driver->driver_op->fs_delete_node)
   {
      /*The filesystem driver does not support this method*/
      return -1;
   }
   ret = driver->driver_op->fs_delete_node(target_inode->parent_node, target_inode);
   ASSERT_MSG(0 == ret, "unlink(): lower layer unlink failed");
   if(ret)
   {
      /* Filesystem op failed */
      return -1;
   }
   /* TODO: Remove node from vfs */
   ret = vfs_delete_child(target_inode);
   ASSERT_MSG(0 == ret, "unlink(): vfs_delete_child() failed");
   if(ret)
   {
      /* Filesystem op failed */
      return -1;
   }
   return 0;
}

extern int filesystem_create(filesystem_info_t **fs, Device dev, filesystem_driver_t *drv)
{
   int ret = -1;

   if(NULL != ((*fs) = (filesystem_info_t *) tlsf_malloc(fs_mem_handle, sizeof(filesystem_info_t))))
   {
      (*fs)->drv = drv;
      (*fs)->device = dev;
      (*fs)->down_layer_info = NULL;
      (*fs)->ref_count = 0;
      ret = 0;
   }
   else
   {

   }
   return ret;
}

/*
MmcBlkDevInitWithSPI(&mmc0, SPI0, 5000000);

SPI0 ya lo inicializastes antes con la frecuencia de trabajo, o lo inicializa sola la capa de blockdev de MMC ya que debe hacer scaling de frecuencia (creo que eso es para tier2, y no me pondria a trabajar en esa feature hasta que no estemos leyendo bloques de una SD a travez de SPI)

Se puede castear?
int format(BlockDev_t *dev, fs_t *fs, void *param)

mmc0_blockdev = ooc_get_interface( (Object) mmc0 );
if( drawable )
drawable->rotate( (Object) self );

int format(mmc0_blockdev, ext2_fs, void *param)
Nada que decir, el format es correto
*/

/** @} doxygen end group definition */
/** @} doxygen end group definition */
/*==================[end of file]============================================*/
