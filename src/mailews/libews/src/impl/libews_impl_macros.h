/*
 * libews_impl_macros.h
 *
 *  Created on: Jul 31, 2014
 *      Author: stone
 */

#ifndef LIBEWS_IMPL_MACROS_H_
#define LIBEWS_IMPL_MACROS_H_

#define IMPL_PROPERTY_GETTER(t, p) \
	virtual t Get##p() const \
	{ \
		return m_##p; \
	}

#define IMPL_PROPERTY_GETTER_B(t, p) \
	virtual t Is##p() const \
	{ \
		return m_##p; \
	}

#define IMPL_PROPERTY_GETTER_B_2(t, p) \
	virtual t Has##p() const \
	{ \
		return m_Has##p; \
	}

#define IMPL_PROPERTY_SETTER(t, p) \
	virtual void Set##p(t value) \
	{ \
		m_##p = value; \
	}

#define IMPL_PROPERTY_SETTER_B_2(t, p) \
	virtual void SetHas##p(t value) \
	{ \
		m_Has##p = value; \
	}

#define IMPL_PROPERTY_GETTER_2(t, p) \
	virtual const t & Get##p() const \
	{ \
		return m_##p; \
	}

#define IMPL_PROPERTY_SETTER_2(t, p) \
	virtual void Set##p(const t & value) \
	{ \
		m_##p = value; \
	}

#define IMPL_PROPERTY_GETTER_NOT_OWN_PT(t, p) \
	virtual t Get##p() const \
	{ \
		if (m_##p) \
			return m_##p; \
		return m_owned_##p; \
	}

#define IMPL_PROPERTY_SETTER_NOT_OWN_PT(t, p) \
	virtual void Set##p(t value) \
	{ \
		if (m_owned_##p) { \
			delete m_owned_##p; \
			m_owned_##p = NULL; \
		} \
		m_##p = value; \
	}

#define IMPL_PROPERTY_FIELD_NOT_OWN_PT(t, p) \
	t m_##p; \
	t m_owned_##p;

#define IMPL_PROPERTY_FIELD(t, p) \
	t m_##p;

#define IMPL_PROPERTY(t, p) \
	private: \
	IMPL_PROPERTY_FIELD(t,p); \
	public : \
	IMPL_PROPERTY_GETTER(t,p); \
	IMPL_PROPERTY_SETTER(t,p);

#define IMPL_PROPERTY_2(t, p) \
	private: \
	IMPL_PROPERTY_FIELD(t,p); \
	public : \
	IMPL_PROPERTY_GETTER_2(t,p); \
	IMPL_PROPERTY_SETTER_2(t,p);

#define IMPL_PROPERTY_B(t, p) \
	private: \
	IMPL_PROPERTY_FIELD(t,p); \
	public : \
	IMPL_PROPERTY_GETTER_B(t,p); \
	IMPL_PROPERTY_SETTER(t,p);

#define IMPL_PROPERTY_B_2(t, p) \
	private: \
	IMPL_PROPERTY_FIELD(t,Has##p); \
	public : \
	IMPL_PROPERTY_GETTER_B_2(t,p); \
	IMPL_PROPERTY_SETTER_B_2(t,p);

#define IMPL_PROPERTY_NOT_OWN_PT(t, p) \
	private: \
	IMPL_PROPERTY_FIELD_NOT_OWN_PT(t,p); \
	public : \
	IMPL_PROPERTY_GETTER_NOT_OWN_PT(t,p); \
	IMPL_PROPERTY_SETTER_NOT_OWN_PT(t,p);

#define INIT_PROPERTY(p, v) \
	m_##p(v)

#define INIT_PROPERTY_B_2(p, v) \
	m_Has##p(v)

#define INIT_PROPERTY_NOT_OWN_PT(p, v) \
	m_owned_##p(v), m_##p(NULL)

#define RELEASE_PROPERTY_NOT_OWN_PT(p) \
	if (m_owned_##p) delete m_owned_##p;

#define DEF_CREATE_INSTANCE(t)                  \
    t * t::CreateInstance() {                   \
        return new t##GsoapImpl();              \
    }

#endif /* LIBEWS_IMPL_MACROS_H_ */
