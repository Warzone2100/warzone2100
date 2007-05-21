typedef struct { Sint32 x, y; } Vector2i;
typedef struct { float x, y; } Vector2f;
typedef struct { Sint32 x, y, z; } Vector3i;
typedef struct { float x, y, z; } Vector3f;


/*!
 * Set the vector field by field, same as v = (Vector3f){x, y, z};
 * Needed for MSVC which doesn't support C99 struct assignments.
 * \param[out] v Vector to set
 * \param[in] x,y,z Values to set to
 */
static inline void Vector3f_Set(Vector3f* v, float x, float y, float z)
{
	v->x = x;
	v->y = y;
	v->z = z;
}


/*!
 * Set the vector field by field, same as v = (Vector3i){x, y, z};
 * Needed for MSVC which doesn't support C99 struct assignments.
 * \param[out] v Vector to set
 * \param[in] x,y,z Values to set to
 */
static inline void Vector3i_Set(Vector3i *v, int x, int y, int z)
{
	v->x = x;
	v->y = y;
	v->z = z;
}


/*!
 * Substract op2 from op1 and store the result in dest.
 * \param[out] dest Result
 * \param[in] op1,op2 Operands
 */
static inline void Vector3f_Sub(Vector3f* dest, Vector3f* op1, Vector3f* op2)
{
	dest->x = op1->x - op2->x;
	dest->y = op1->y - op2->y;
	dest->z = op1->z - op2->z;
}


/*!
 * Calculate the scalar product of op1 and op2.
 * \param[in] op1,op2 Operands
 * \return Scalarproduct of the 2 vectors
 */
static inline float Vector3f_SP(Vector3f* op1, Vector3f* op2)
{
	return op1->x * op2->x + op1->y * op2->y + op1->z * op2->z;
}


/*!
 * Calculate the crossproduct of op1 and op2 and store the result in dest.
 * \param[out] dest Result
 * \param[in] op1,op2 Operands
 */
static inline void Vector3f_CP(Vector3f* dest, Vector3f* op1, Vector3f* op2)
{
	dest->x = op1->y * op2->z - op1->z * op2->y;
	dest->y = op1->z * op2->x - op1->x * op2->z;
	dest->z = op1->x * op2->y - op1->y * op2->x;
}
