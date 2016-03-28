/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/

#ifndef TM_REGISTER_INTERFACE_H
#define TM_REGISTER_INTERFACE_H


void __set_field(void * address, int shift, int width, int value);

unsigned int __get_field(void * address, int shift, int width);


/* workaround for MSVS variadic macros bug - all variadic parameters are passed inside macro as 1 parameter (first in list) other are empty
 so I use __VA_ARGS__ inside macro  and following macros in order to extract parameters from VA_ARGS_
 The following dummy macro is used because this action needs additional macro nesting level (???)
*/
#define DUMMY_MACRO(x) x

#define PARAM_1_OF_4(a, b, c, d) a
#define PARAM_2_OF_4(a, b, c, d) b
#define PARAM_3_OF_4(a, b, c, d) c
#define PARAM_4_OF_4(a, b, c, d) d

#define PARAM_1_OF_3(a, b, c) a
#define PARAM_2_OF_3(a, b, c) b
#define PARAM_3_OF_3(a, b, c) c

#define PARAM_1_OF_2(a, b) a
#define PARAM_2_OF_2(a, b) b

/* intermediate macros */

#define	 TM_REG_ROW_SET(field_name, shift, width, default_value, ...)\
	(else\
	if (strcmp(DUMMY_MACRO(PARAM_1_OF_3(__VA_ARGS__)), #field_name) == 0) {\
		__set_field(TM_REGISTER_VAR_PTR(PARAM_2_OF_3(__VA_ARGS__)), shift,\
		width, DUMMY_MACRO(PARAM_3_OF_3(__VA_ARGS__)));\
	})

#define	 TM_REG_ROW_RESET(field_name, shift, width, default_value, ...)\
	__set_field(TM_REGISTER_VAR_PTR(__VA_ARGS__), shift, width, default_value);

#define	 TM_REG_ROW_GET(field_name, shift, width, default_value, ...)\
	(else\
	if (strcmp(DUMMY_MACRO(PARAM_1_OF_4(__VA_ARGS__)), #field_name) == 0) {\
		DUMMY_MACRO(PARAM_3_OF_4(__VA_ARGS__)) = \
		PARAM_4_OF_4(__VA_ARGS__)__get_field(TM_REGISTER_VAR_PTR(PARAM_2_OF_4(__VA_ARGS__)), shift, width);\
	})

#define	 TM_REG_ROW_DUMP(field_name, shift, width, default_value, ...)\
		fprintf(DUMMY_MACRO(PARAM_2_OF_2(__VA_ARGS__)), "    %s (shift=%d, width=%d) value=%u\n",\
		#field_name, shift, width, __get_field(TM_REGISTER_VAR_PTR(PARAM_1_OF_2(__VA_ARGS__)), shift, width));



/* the register variable of some register type must be unique in the scope of it's definition
   It's name is defined by register name in the following macro.
   Two following macros should be used everywhere instead of direct usage of
   refister variable name and address
*/
#define	TM_REGISTER_VAR_NAME(register) register##_var
#define	TM_REGISTER_VAR_ADDR(register) TM_REGISTER_VAR_PTR(TM_REGISTER_VAR_NAME(register))

/* if use register description using shifts: */
#if defined(SHIFT_TABLE)
	/* the register variable is defined as 8 bytes memory block */
	#define	TM_REGISTER_VAR(register_name) char TM_REGISTER_VAR_NAME(register_name)[8];
	#define	TM_REGISTER_VAR_PTR(register_var)  (void*)register_var

	#define	 TM_REGISTER_SET(register, param_name, param_value) \
	{\
		if (0) ;\
		register(SET, #param_name, register, param_value)\
		else { \
		fprintf(stderr, "%s line:%d unknown parameter %s used assignment. aboriting.\n",\
		__FILE__, __LINE__, #param_name);	exit(1); } \
	}

	#define	 TM_REGISTER_GET(register, param_name, param_value,casting) \
	{\
		if (0) ;\
		register(GET, #param_name, register, param_value, casting)\
		else { \
		fprintf(stderr, "%s line:%d unknown parameter %s used in read request. aboriting.\n",\
		__FILE__, __LINE__, #param_name);	exit(1); } \
	}

	#define	TM_REGISTER_RESET(register) \
	{\
		memset(TM_REGISTER_VAR_NAME(register), 0, sizeof(char)*8);\
		register(RESET, register)\
	}

	#define	 TM_REGISTER_DUMP(register, file_handle) \
	{\
		fprintf(file_handle, " register %s:\n", #register);\
		register(DUMP, register, file_handle)\
	}

/* if use register description using  structure declaration: */

#elif defined(STRUCTS)

	#define STRUCTURE_INITIALIZATION_CODE = {0}
	#define	TM_REGISTER_VAR(register)  struct register TM_REGISTER_VAR_NAME(register) STRUCTURE_INITIALIZATION_CODE;
	#define	TM_REGISTER_VAR_PTR(register_var)  (void*)&register_var

	#define	TM_REGISTER_SET(register, param_name, param_value) {\
		TM_REGISTER_VAR_NAME(register).param_name = param_value; }
	#define	TM_REGISTER_GET(register, param_name, param_value, casting) {\
		param_value = casting TM_REGISTER_VAR_NAME(register).param_name; }

	#define	TM_REGISTER_RESET(register)  /* nothing to do */
	#define	TM_REGISTER_DUMP(register_name, file_handle) /* nothing to do */
#else
/* error here - data declaration method must be selected */
#endif

#endif   /* TM_REGISTER_INTERFACE_H */
