/**
 * @file synctools.h
 *
 * @author eric.brunet@parrot.com
 *
 * @copyright Copyright (C) 2016 Parrot S.A.
 */

#ifndef _SYNCTOOLS_H_
#define _SYNCTOOLS_H_

/**
 * @brief Synchronize a file
 *
 * @param[in] path The targeted file
 *
 * @return 0 on success
 *         negative errno on error
 */
int sync_file(const char *filepath);

/**
 * @brief Synchronize a folder
 *
 * @param[in] path The targeted folder
 *
 * @return 0 on success
 *         negative errno on error
 */
int sync_folder(const char *folderpath);

/**
 * @brief Synchronize a file and it's parent folder
 *
 * @param[in] path The targeted file
 *
 * @return 0 on success
 *         negative errno on error
 */
int sync_file_and_folder(const char *filepath);

#endif /* _SYNCTOOLS_H_ */
