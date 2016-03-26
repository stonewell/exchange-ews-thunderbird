#pragma once
#ifndef __LIBEWS_GSOAP_DEFS_H__
#define __LIBEWS_GSOAP_DEFS_H__

#define SET_PROPERTY(x, y, p)                   \
	if (x->p)                                   \
		y->Set##p(*x->p);

#define SET_PROPERTY_2(x, y, p, q)              \
	if (x->p)                                   \
		y->Set##p((q)(*x->p));

#define SET_PROPERTY_3(x, y, p, q)              \
	y->Set##p((q)(x->p));

#define SET_PROPERTY_STR(x, y, p)               \
	if (x->p)                                   \
		y->Set##p(x->p->c_str());

#define SET_PROPERTY_B(x, y, p)                 \
	if (x->Is##p)                               \
		y->Set##p(*x->Is##p);

#define SET_PROPERTY_ARRAY(x, y, p)             \
	if (x->p)                                   \
		SetArrayProperty(y->Get##p(), x->p);

#define FROM_PROPERTY_STR_4(v_s, v_t, pool, p_name)                     \
    if (!v_s->Get##p_name().IsEmpty())                                  \
    {                                                                   \
        v_t->p_name = pool->Create<std::string>(v_s->Get##p_name().GetData()); \
    }

#define FROM_PROPERTY_STR(p_name)                                       \
    FROM_PROPERTY_STR_4(pEwsCalendar, pCalendar, m_pObjPool, p_name)

#define FROM_PROPERTY_6(v_s, v_t, pool, p_name, t, default_value)   \
    if (v_s->Get##p_name() != default_value)                        \
    {                                                               \
        FROM_PROPERTY_5(v_s, v_t, pool, p_name, t)                  \
                }

#define FROM_PROPERTY_3(p_name, t, default_value)       \
    if (pEwsCalendar->Get##p_name() != default_value)   \
    {                                                   \
        FROM_PROPERTY_2(p_name, t)                      \
                }

#define FROM_PROPERTY_5(v_s, v_t, pool, p_name, t)  \
    {                                               \
        v_t->p_name = pool->Create<t>();            \
        *v_t->p_name = (t)v_s->Get##p_name();       \
    }

#define FROM_PROPERTY_2(p_name, t)                                  \
    FROM_PROPERTY_5(pEwsCalendar, pCalendar, m_pObjPool, p_name, t)

#define FROM_PROPERTY_BOOL(p_name)                          \
    {                                                       \
        pCalendar->p_name = m_pObjPool->Create<bool>();     \
        *pCalendar->p_name = (bool)pEwsCalendar->p_name();  \
    }

#define FROM_PROPERTY_BOOL_2(p_name)                            \
    {                                                           \
        pCalendar->p_name = m_pObjPool->Create<bool>();         \
        *pCalendar->p_name = (bool)pEwsCalendar->Is##p_name();  \
    }

#define FROM_PROPERTY_RECIPIENT(p_name)                                 \
    if (pEwsCalendar->Get##p_name()) {                                  \
        pCalendar->p_name = m_pObjPool->Create<ews2__SingleRecipientType>(); \
        ToRecipient(pEwsCalendar->Get##p_name(),                        \
                    pCalendar->p_name, m_pObjPool);                     \
    }

#define FROM_PROPERTY_ITEM_ID(v_s, v_t, pool)           \
    if (!v_s->GetItemId().IsEmpty()) {                  \
        v_t->ItemId =                                   \
                pObjPool->Create<ews2__ItemIdType>();   \
        v_t->ItemId->Id =                               \
                v_s->GetItemId().GetData();             \
        if (!v_s->GetChangeKey().IsEmpty()) {           \
            FROM_PROPERTY_STR_4(v_s,                    \
                                v_t->ItemId,            \
                                pObjPool,               \
                                ChangeKey);             \
        }                                               \
    }

#define FROM_ARRAY_PROPERTY(p_name, t)                              \
    if (pEwsCalendar->Get##p_name()->size() > 0) {                  \
        pCalendar->p_name = m_pObjPool->Create<t>();                \
        FromArrayProperty(pCalendar->p_name,                        \
                          pEwsCalendar->Get##p_name(), m_pObjPool); \
    } else {                                                        \
        pCalendar->p_name = NULL;                                   \
    }

#define FROM_RECURRENCE(p_name)  \
    FROM_RECURRENCE_4(pCalendar, pEwsCalendar, p_name, ews2__RecurrenceType)

#define FROM_RECURRENCE_2(p_name,t)                           \
    FROM_RECURRENCE_4(pCalendar, pEwsCalendar, p_name, t)

#define FROM_RECURRENCE_4(item, pItem, p_name, t)                         \
    if (pItem->Get##p_name())                                           \
    {                                                                   \
        item->p_name = m_pObjPool->Create<t>();      \
        FromRecurrence(item->p_name, pItem->Get##p_name(), m_pObjPool); \
    }

#define FROM_OCCURRENCE(p_name)                                         \
    if (pEwsCalendar->Get##p_name())                                    \
    {                                                                   \
        pCalendar->p_name = m_pObjPool->Create<ews2__OccurrenceInfoType>(); \
        FromOccurrenceInfo(pCalendar->p_name, pEwsCalendar->Get##p_name(), m_pObjPool); \
    }


#define FROM_TIMEZONE(p_name)                                           \
    if (pEwsCalendar->Get##p_name())                                    \
    {                                                                   \
        pCalendar->p_name = m_pObjPool->Create<ews2__TimeZoneType>();   \
        FromTimeZone(pCalendar->p_name, pEwsCalendar->Get##p_name(), m_pObjPool); \
    }

#define ARRAY_OF_STRING_TYPE_TO_STRING_LIST(x, y, p)    \
	if (x->p) {                                         \
		y->Set##p(new CEWSStringList());                \
		ArrayOfStringTypeToStringList(x->p,             \
                                      y->Get##p());     \
	}

inline
void
ArrayOfStringTypeToStringList(ews2__ArrayOfStringsType * pType,
                              CEWSStringList * pStringList) {
	size_t count = pType->String.size();

	for(size_t i=0; i < count; i++) {
		pStringList->push_back(pType->String[i].c_str());
	}
}

#define STRING_LIST_TO_ARRAY_OF_STRING_TYPE(x, y, p)   \
	if (y->Get##p()) {                                  \
        x->p = m_pObjPool->Create<ews2__ArrayOfStringsType>();      \
		StringListToArrayOfStringType(x->p,             \
                                      y->Get##p());     \
	}

inline
void
StringListToArrayOfStringType(ews2__ArrayOfStringsType * pType,
                              CEWSStringList * pStringList) {
	size_t count = pStringList->size();

	for(size_t i=0; i < count; i++) {
        pType->String.push_back(pStringList->at(i).GetData());
	}
}

inline
void
From(CEWSRecurrencePatternRelativeYearly * pp,
     ews2__RelativeYearlyRecurrencePatternType * item) {
	SET_PROPERTY_3(item,
	               pp,
	               DaysOfWeek,
	               enum CEWSRecurrencePattern::EWSDayOfWeekEnum);
	SET_PROPERTY_3(item,
	               pp,
	               DayOfWeekIndex,
	               enum CEWSRecurrencePattern::EWSDayOfWeekIndexEnum);
	SET_PROPERTY_3(item,
	               pp,
	               Month,
	               enum CEWSRecurrencePattern::EWSMonthNamesEnum);
}

template<class T>
CEWSRecurrencePattern * PatternFrom(int t,
                                    T * item) {
	CEWSRecurrencePattern * p = NULL;

	switch(t) {
	case SOAP_UNION__ews2__union_RecurrenceType_RelativeYearlyRecurrence: {
		CEWSRecurrencePatternRelativeYearly * pp =
				new CEWSRecurrencePatternRelativeYearlyGsoapImpl;

		From(pp, item->RelativeYearlyRecurrence);
		p = pp;
	}
		break;
	case SOAP_UNION__ews2__union_RecurrenceType_AbsoluteYearlyRecurrence: {
		CEWSRecurrencePatternAbsoluteYearly * pp =
				new CEWSRecurrencePatternAbsoluteYearlyGsoapImpl;
		SET_PROPERTY_3(item->AbsoluteYearlyRecurrence,
		               pp,
		               Month,
		               enum CEWSRecurrencePattern::EWSMonthNamesEnum);
		SET_PROPERTY_3(item->AbsoluteYearlyRecurrence,
		               pp,
		               DayOfMonth,
		               int);
		p = pp;
	}
		break;
	case SOAP_UNION__ews2__union_RecurrenceType_RelativeMonthlyRecurrence: {
		CEWSRecurrencePatternRelativeMonthly * pp =
				new CEWSRecurrencePatternRelativeMonthlyGsoapImpl;
		SET_PROPERTY_3(item->RelativeMonthlyRecurrence,
		               pp,
		               DaysOfWeek,
		               enum CEWSRecurrencePattern::EWSDayOfWeekEnum);
		SET_PROPERTY_3(item->RelativeMonthlyRecurrence,
		               pp,
		               DayOfWeekIndex,
		               enum CEWSRecurrencePattern::EWSDayOfWeekIndexEnum);
		SET_PROPERTY_3(item->RelativeMonthlyRecurrence,
		               pp,
		               Interval,
		               int);
		p = pp;
	}
		break;
	case SOAP_UNION__ews2__union_RecurrenceType_AbsoluteMonthlyRecurrence: {
		CEWSRecurrencePatternAbsoluteMonthly * pp =
				new CEWSRecurrencePatternAbsoluteMonthlyGsoapImpl;
		SET_PROPERTY_3(item->AbsoluteMonthlyRecurrence,
		               pp,
		               DayOfMonth,
		               int);
		SET_PROPERTY_3(item->AbsoluteMonthlyRecurrence,
		               pp,
		               Interval,
		               int);
		p = pp;
	}
		break;
	case SOAP_UNION__ews2__union_RecurrenceType_WeeklyRecurrence: {
		CEWSRecurrencePatternWeekly * pp =
				new CEWSRecurrencePatternWeeklyGsoapImpl;
		SET_PROPERTY_3(item->WeeklyRecurrence,
		               pp,
		               DaysOfWeek,
		               enum CEWSRecurrencePattern::EWSDayOfWeekEnum);
		SET_PROPERTY_3(item->WeeklyRecurrence,
		               pp,
		               Interval,
		               int);
		p = pp;
	}
		break;
	case SOAP_UNION__ews2__union_RecurrenceType_DailyRecurrence: {
		CEWSRecurrencePatternDaily * pp =
				new CEWSRecurrencePatternDailyGsoapImpl;
		SET_PROPERTY_3(item->DailyRecurrence,
		               pp,
		               Interval,
		               int);
		p = pp;
	}
		break;
	default:
		break;
	}

	return p;
}

template<class T>
CEWSRecurrenceRange * RangeFrom(int t,
                                T * item) {
	CEWSRecurrenceRange * p = NULL;

	switch(t) {
	case SOAP_UNION__ews2__union_RecurrenceType__NoEndRecurrence: {
		CEWSNoEndRecurrenceRange * pp =
				new CEWSNoEndRecurrenceRangeGsoapImpl;

		pp->SetStartDate(item->NoEndRecurrence->StartDate.c_str());
		p = pp;
	}
		break;
	case SOAP_UNION__ews2__union_RecurrenceType__EndDateRecurrence: {
		CEWSEndDateRecurrenceRange * pp =
				new CEWSEndDateRecurrenceRangeGsoapImpl;
		pp->SetStartDate(item->EndDateRecurrence->StartDate.c_str());
		pp->SetEndDate(item->EndDateRecurrence->EndDate.c_str());
		p = pp;
	}
		break;
	case SOAP_UNION__ews2__union_RecurrenceType__NumberedRecurrence: {
		CEWSNumberedRecurrenceRange * pp =
				new CEWSNumberedRecurrenceRangeGsoapImpl;
		pp->SetStartDate(item->NumberedRecurrence->StartDate.c_str());
		pp->SetNumberOfOccurrences(item->NumberedRecurrence->NumberOfOccurrences);
		p = pp;
	}
		break;
	default:
		break;
	}

	return p;
}

template<class T>
void FromRecurrenceRange(CEWSRecurrenceRange * p,
                         int & rangeType,
                         T & r,
                         CEWSObjectPool * pObjPool) {
    if (!p) {
        return;
    }
    switch(p->GetRecurrenceRangeType()) {
    case CEWSRecurrenceRange::NoEnd:
        r.NoEndRecurrence =
                pObjPool->Create<ews2__NoEndRecurrenceRangeType>();
        r.NoEndRecurrence->StartDate =
                p->GetStartDate();
        rangeType =
                SOAP_UNION__ews2__union_RecurrenceType__NoEndRecurrence;
        break;
    case CEWSRecurrenceRange::EndDate:
        r.EndDateRecurrence =
                pObjPool->Create<ews2__EndDateRecurrenceRangeType>();
        r.EndDateRecurrence->StartDate =
                p->GetStartDate();
        r.EndDateRecurrence->EndDate =
                dynamic_cast<CEWSEndDateRecurrenceRange*>(p)->GetEndDate();
        rangeType =
                SOAP_UNION__ews2__union_RecurrenceType__EndDateRecurrence;
        break;
    case CEWSRecurrenceRange::Numbered:
        r.NumberedRecurrence =
                pObjPool->Create<ews2__NumberedRecurrenceRangeType>();
        r.NumberedRecurrence->StartDate =
                p->GetStartDate();
        r.NumberedRecurrence->NumberOfOccurrences =
                dynamic_cast<CEWSNumberedRecurrenceRange*>(p)->GetNumberOfOccurrences();
        rangeType =
                SOAP_UNION__ews2__union_RecurrenceType__NumberedRecurrence;
        break;
    default:
        break;
    }
}

template<class T>
void FromRecurrencePattern(CEWSRecurrencePatternRelativeYearly * p,
                           int & patternType,
                           T & r,
                           CEWSObjectPool * pObjPool) {
    patternType =
            SOAP_UNION__ews2__union_RecurrenceType_RelativeYearlyRecurrence;
    r.RelativeYearlyRecurrence =
            pObjPool->Create<ews2__RelativeYearlyRecurrencePatternType>();
    r.RelativeYearlyRecurrence->DaysOfWeek =
            (enum ews2__DayOfWeekType)p->GetDaysOfWeek();
    r.RelativeYearlyRecurrence->DayOfWeekIndex =
            (enum ews2__DayOfWeekIndexType)p->GetDayOfWeekIndex();
    r.RelativeYearlyRecurrence->Month =
            (enum ews2__MonthNamesType)p->GetMonth();
}

template<class T>
void FromRecurrencePattern(CEWSRecurrencePatternAbsoluteYearly * p,
                           int & patternType,
                           T & r,
                           CEWSObjectPool * pObjPool) {
    patternType =
            SOAP_UNION__ews2__union_RecurrenceType_AbsoluteYearlyRecurrence;
    r.AbsoluteYearlyRecurrence =
            pObjPool->Create<ews2__AbsoluteYearlyRecurrencePatternType>();
    r.AbsoluteYearlyRecurrence->DayOfMonth =
            p->GetDayOfMonth();
    r.AbsoluteYearlyRecurrence->Month =
            (enum ews2__MonthNamesType)p->GetMonth();
}

template<class T>
void FromRecurrencePattern(CEWSRecurrencePatternRelativeMonthly * p,
                           int & patternType,
                           T & r,
                           CEWSObjectPool * pObjPool) {
    patternType =
            SOAP_UNION__ews2__union_RecurrenceType_RelativeMonthlyRecurrence;
    r.RelativeMonthlyRecurrence =
            pObjPool->Create<ews2__RelativeMonthlyRecurrencePatternType>();
    r.RelativeMonthlyRecurrence->DaysOfWeek =
            (enum ews2__DayOfWeekType)p->GetDaysOfWeek();
    r.RelativeMonthlyRecurrence->DayOfWeekIndex =
            (enum ews2__DayOfWeekIndexType)p->GetDayOfWeekIndex();
    r.RelativeMonthlyRecurrence->Interval =
            p->GetInterval();
}

template<class T>
void FromRecurrencePattern(CEWSRecurrencePatternAbsoluteMonthly * p,
                           int & patternType,
                           T & r,
                           CEWSObjectPool * pObjPool) {
    patternType =
            SOAP_UNION__ews2__union_RecurrenceType_AbsoluteMonthlyRecurrence;
    r.AbsoluteMonthlyRecurrence =
            pObjPool->Create<ews2__AbsoluteMonthlyRecurrencePatternType>();
    r.AbsoluteMonthlyRecurrence->DayOfMonth =
            p->GetDayOfMonth();
    r.AbsoluteMonthlyRecurrence->Interval =
            p->GetInterval();
}

template<class T>
void FromRecurrencePattern(CEWSRecurrencePatternWeekly * p,
                           int & patternType,
                           T & r,
                           CEWSObjectPool * pObjPool) {
    patternType =
            SOAP_UNION__ews2__union_RecurrenceType_WeeklyRecurrence;
    r.WeeklyRecurrence =
            pObjPool->Create<ews2__WeeklyRecurrencePatternType>();
    r.WeeklyRecurrence->DaysOfWeek =
            (enum ews2__DaysOfWeekType)p->GetDaysOfWeek();
    r.WeeklyRecurrence->Interval =
            p->GetInterval();
}

template<class T>
void FromRecurrencePattern(CEWSRecurrencePatternDaily * p,
                           int & patternType,
                           T & r,
                           CEWSObjectPool * pObjPool) {
    patternType =
            SOAP_UNION__ews2__union_RecurrenceType_DailyRecurrence;
    r.DailyRecurrence =
            pObjPool->Create<ews2__DailyRecurrencePatternType>();
    r.DailyRecurrence->Interval =
            p->GetInterval();
}

template<class T>
void FromRecurrencePattern(CEWSRecurrencePattern * p,
                           int & patternType,
                           T & r,
                           CEWSObjectPool * pObjPool) {
    if (!p) return;

    switch(p->GetRecurrenceType()) {
    case CEWSRecurrencePattern::RelativeYearly:
        FromRecurrencePattern(dynamic_cast<CEWSRecurrencePatternRelativeYearly*>(p),
                              patternType,
                              r,
                              pObjPool);
        break;
    case CEWSRecurrencePattern::AbsoluteYearly:
        FromRecurrencePattern(dynamic_cast<CEWSRecurrencePatternAbsoluteYearly*>(p),
                              patternType,
                              r,
                              pObjPool);
        break;
    case CEWSRecurrencePattern::RelativeMonthly:
        FromRecurrencePattern(dynamic_cast<CEWSRecurrencePatternRelativeMonthly*>(p),
                              patternType,
                              r,
                              pObjPool);
        break;
    case CEWSRecurrencePattern::AbsoluteMonthly:
        FromRecurrencePattern(dynamic_cast<CEWSRecurrencePatternAbsoluteMonthly*>(p),
                              patternType,
                              r,
                              pObjPool);
        break;
    case CEWSRecurrencePattern::Weekly:
        FromRecurrencePattern(dynamic_cast<CEWSRecurrencePatternWeekly*>(p),
                              patternType,
                              r,
                              pObjPool);
        break;
    case CEWSRecurrencePattern::Daily:
        FromRecurrencePattern(dynamic_cast<CEWSRecurrencePatternDaily*>(p),
                              patternType,
                              r,
                              pObjPool);
        break;
    }
}

#endif //__LIBEWS_GSOAP_DEFS_H__
