/*****
*
* Copyright (C) 2000 Yoann Vandoorselaere <yoann@mandrakesoft.com>
* All Rights Reserved
*
* This file is part of the Prelude program.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by 
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/




/*
 * If the authentication file 'filename' do not exist,
 * create it, asking for the creation of a new user.
 *
 * crypted mean if the password will be written in a
 * crypted fashion or not.
 */
int auth_file_exist_or_create(const char *filename, int crypted);




/*
 * read the authentication file pointed by fd,
 * store the current line, user, and pass in their
 * corresponding pointer.
 *
 * If not NULL, user & pass pointer must be freed.
 */
int auth_read_entry(FILE *fd, int *line, char **user, char **pass);




/*
 * Ask for a new account creation which will be stored
 * into 'filename' which is the authentication file.
 *
 * crypted mean if the password will be written in
 * a crypted fashion or not.
 */
int auth_create_account(const char *filename, int crypted);


#define SALT "ak"
