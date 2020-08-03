/** \file
 * \brief Tests for geometry.h
 *
 * \author Max Ilsen
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <ogdf/basic/geometry.h>
#include <testing.h>

go_bandit([]() {
	describe("DPolyline", [](){
		describe("normalize, trivial cases", [](){
			it("works on an empty polyline", [](){
				DPolyline dpline;
				dpline.normalize();
				AssertThat(dpline, Equals(dpline));
			});

			it("works on a polyline with one point", [](){
				DPolyline dpline({DPoint(1,1)});
				dpline.normalize();
				AssertThat(dpline, Equals(dpline));
			});

			it("works on a polyline with two points", [](){
				DPolyline dpline({DPoint(1,1), DPoint(2,2)});
				dpline.normalize();
				AssertThat(dpline, Equals(dpline));
			});

			it("works on a polyline with three points", [](){
				DPolyline dpline({DPoint(1,1), DPoint(2,2), DPoint(3,3)});
				DPolyline result({DPoint(1,1), DPoint(3,3)});
				dpline.normalize();
				AssertThat(dpline, Equals(result));
			});
		});

		describe("normalize, non-trivial cases", [](){
			DPoint p0(0,0);
			DPoint p1(1,1); // 180 degree
			DPoint p2(2,2); // 180 degree
			DPoint p3(3,3); // 135 degree
			DPoint p4(3,4); // 90 degree
			DPoint p5(4,4); // 90 degree
			DPoint p6(4,6); // 45 degree
			DPoint p7(5,5); // 45 degree
			DPoint p8(5,6); // 135 degree
			DPoint p9(6,7); // 135 degree
			DPoint p10(7,7); // 180 degree
			DPoint p11(8,7); // 180 degree
			DPoint p12(9,7); // 90 degree
			DPoint p13(9,8);
			DPolyline dpline;

			before_each([&](){
				dpline = DPolyline({p1, p2, p3, p4,  p5,  p6,
				                    p7, p8, p9, p10, p11, p12});
			});

			it("works without parameters", [&](){
				DPolyline result({p1, p3, p4, p5, p6, p7, p8, p9, p12});
				dpline.normalize();
				AssertThat(dpline, Equals(result));
			});

			it("works with a minimum angle of 3/4 Pi", [&](){
				DPolyline result({p1, p4, p5, p6, p7, p9, p12});
				dpline.normalize(0.75*Math::pi);
				AssertThat(dpline, Equals(result));
			});

			it("works with a minimum angle of 1/2 Pi", [&](){
				DPolyline result({p1, p12});
				dpline.normalize(Math::pi_2);
				AssertThat(dpline, Equals(result));
			});

			it("works with source/target points", [&](){
				DPolyline result({p3, p4, p5, p6, p7, p8, p9, p12});
				dpline.normalize(p0, p13);
				AssertThat(dpline, Equals(result));
			});

			it("works with source/target points and a minimum angle of 3/4 Pi", [&](){
				DPolyline result({p4, p5, p6, p7, p9, p12});
				dpline.normalize(p0, p13, 0.75*Math::pi);
				AssertThat(dpline, Equals(result));
			});

			it("works with source/target points and a minimum angle of 1/2 Pi", [&](){
				dpline.normalize(p0, p13, Math::pi_2);
				AssertThat(dpline.empty(), IsTrue());
			});

			it("works with source/target points equaling the polyline's first/last point", [&](){
				DPolyline result({p3, p4, p5, p6, p7, p8, p9});
				dpline.normalize(DPoint(p1), DPoint(p12));
				AssertThat(dpline, Equals(result));
			});
		});
	});
});
