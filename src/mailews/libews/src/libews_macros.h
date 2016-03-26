/*
 * libews_macros.h
 *
 *  Created on: Jul 24, 2014
 *      Author: stone
 */

#ifndef LIBEWS_MACROS_H_
#define LIBEWS_MACROS_H_

#define DECLARE_PROPERTY_GETTER(t,p) \
	virtual t Get##p() const = 0;

#define DECLARE_PROPERTY_GETTER_B(t,p) \
	virtual t Is##p() const = 0;

#define DECLARE_PROPERTY_GETTER_B_2(t,p) \
	virtual t Has##p() const = 0;

#define DECLARE_PROPERTY_SETTER(t,p) \
	virtual void Set##p(t value) = 0;

#define DECLARE_PROPERTY_SETTER_B_2(t,p) \
	virtual void SetHas##p(t value) = 0;

#define DECLARE_PROPERTY(t, p) \
	DECLARE_PROPERTY_GETTER(t,p);\
	DECLARE_PROPERTY_SETTER(t,p);

#define DECLARE_PROPERTY_B(t, p) \
	DECLARE_PROPERTY_GETTER_B(t,p);\
	DECLARE_PROPERTY_SETTER(t,p);

#define DECLARE_PROPERTY_B_2(t, p) \
	DECLARE_PROPERTY_GETTER_B_2(t,p);\
	DECLARE_PROPERTY_SETTER_B_2(t,p);

#endif /* LIBEWS_MACROS_H_ */
