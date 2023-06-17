#include <atlsafe.h>
#include <oleauto.h>
#include <iostream>
#include <vector>
#import "femap.tlb" rename("GetProp", "GP")
#include <time.h>
#include <unordered_map>
#include <set>
#include <string>

template <typename T> class Container
{
	private:	
		T* Elem;
		T* last_el;
		T* first_el;
		int size;

	public:
		Container()
		{
			Elem = nullptr;
			last_el = nullptr;
			first_el = nullptr;
			size = 0;
		};

		Container(int el_num)
		{
			Elem = new T[el_num]; 
			first_el = &Elem[0];
			last_el = first_el;
			size = el_num;
		};

		~Container()
		{
			delete[] Elem;
			last_el = nullptr;
			size = 0;
		};

		T operator[] (int index)
		{
			return Elem[index];
		}

		void push_back(T pushed_el)
		{
			*last_el = pushed_el;
			last_el++;
		}

		T* back()
		{
			return last_el;
		}

		T* front()
		{
			return first_el;
		}

		int get_size()
		{
			return size;
		}

		void Set(int el_num)
		{
			Elem = new T[el_num];
			last_el = &Elem[0];
			first_el = last_el;
			size = el_num;
		}

};

class Materials
{
	private:
		int number = 0;
		std::unordered_map<int, const double* > Materials_map;
		Container<double> Characteristics;

	public:

		void Load(femap::ImodelPtr& pModel, CComQIPtr<femap::ISet>& Matl_Set)
		{
			CComQIPtr<femap::IMatl> Matl_obj;
			Matl_obj = pModel->feMatl;
			

			number = Matl_Set->Count();
			Characteristics.Set(number * 5);

			Matl_Set->Reset();
			while (Matl_Set->Next() != femap::zReturnCode::FE_FAIL)
			{
					Matl_obj->Get(Matl_Set->CurrentID);
					
					if (Matl_obj->type == femap::zMaterialType::FMT_ORTHOTROPIC_2D)
						{
							Materials_map.emplace(Matl_obj->ID, Characteristics.back());

							if (Matl_obj->TensionLimit1 != 0)	Characteristics.push_back(Matl_obj->TensionLimit1);
								else std::cout << "Material " << Matl_obj->ID << " Tension Limit 1 = 0";

							if (Matl_obj->TensionLimit2 != 0)	Characteristics.push_back(Matl_obj->TensionLimit2);
								else std::cout << "Material " << Matl_obj->ID << " Tension Limit 2 = 0";

							if (Matl_obj->CompressionLimit1 != 0)	Characteristics.push_back(Matl_obj->CompressionLimit1);		
								else std::cout << "Material " << Matl_obj->ID << " Compression Limit 1 = 0";

							if (Matl_obj->CompressionLimit2 != 0)	Characteristics.push_back(Matl_obj->CompressionLimit2);		
								else std::cout << "Material " << Matl_obj->ID << " Compression Limit 2 = 0";

							if (Matl_obj->ShearLimit != 0)	Characteristics.push_back(Matl_obj->ShearLimit);		
								else std::cout << "Material " << Matl_obj->ID << " ShearLimit = 0";
						}
					else 
					{
						std::cout << "Material type is not ortotropic 2D";
					}

					
			}
		}

		double Get_TensionLimit1(int mat_ID)
		{
			return *(Materials_map[mat_ID]);
		}

		double Get_TensionLimit2(int mat_ID)
		{
			return *(Materials_map[mat_ID] + 1);
		}

		double Get_CompressionLimit1(int mat_ID)
		{
			return *(Materials_map[mat_ID] + 2);
		}

		double Get_CompressionLimit2(int mat_ID)
		{
			return *(Materials_map[mat_ID] + 3);
		}

		double Get_ShearLimit(int mat_ID)
		{
			return *(Materials_map[mat_ID] + 4);
		}

		void Get_Characteristics(int mat_ID, double &matSigXT, double &matSigXC, double &matSigYT, double &matSigYC, double &matTau)
		{
			matSigXT = *(Materials_map[mat_ID]);
			matSigXC = *(Materials_map[mat_ID] + 2);
			matSigYT = *(Materials_map[mat_ID] + 1);
			matSigYC = *(Materials_map[mat_ID] + 3);
			matTau = *(Materials_map[mat_ID] + 4);
		}

		void Out()
		{
			for (std::unordered_map<int, const double* >::const_iterator it = Materials_map.cbegin(); it != Materials_map.cend(); ++it)
			{
				std::cout << Get_TensionLimit1(it->first) << std::endl;
				std::cout << Get_TensionLimit2(it->first) << std::endl;
				std::cout << Get_CompressionLimit1(it->first) << std::endl;
				std::cout << Get_CompressionLimit2(it->first) << std::endl;
				std::cout << Get_ShearLimit(it->first) << std::endl;
			}
		}

};

class Layups
{
	private:
		int number = 0;
		int total_plies_num = 0;
		int ply_characteristics_number = 5; //matID
		std::unordered_map<int, const double* > Layups_map;
		std::unordered_map<int, int* > Elements_map;
		Container<int> Elements_container;
		Container<double> plies;
		femap::zReturnCode rc;
		/*
			Структура контейнера Layups_map

				Layups_map[i]
					|	
					v
			[Layup_1 numOfplies, Layup_1 Ply_1 MatID, Layup_1 Ply_1 Thikness, Layup_1 Ply_1 Angle, Layup_1 Ply_1 GlobalPLy, Layup_1 Ply_1 FailureTheory,
			 Layup_1 Ply_2 MatlID, Layup_1 Ply_2 Thikness, Layup_1 Ply_2 Angle, Layup_1 Ply_2 GlobalPLy, Layup_1 Ply_2 FailureTheory, ...
			 Layup_2 numOfplies, Layup_2 Ply_1 MatID, Layup_2 Ply_1 Thikness, Layup_2 Ply_1 Angle, Layup_2 Ply_1 GlobalPLy, Layup_2 Ply_1 FailureTheory, ...
			 ...]
		*/

		/*
			Структура контейнера Elements_map

				Elements_map
					-
					v
			[Elem_number on Layup_1, Elem_1 (first) on Layup_1 ... Elem_n (last) on Layup_1,
				Elem_number on Layup_2, Elem_1 (first) on Layup_1 ... Elem_n (last) on Layup_1,
				...]
		*/

	public:

		void Load(femap::ImodelPtr& pModel, CComQIPtr<femap::ISet> Layup_Set)
		{
			number = Layup_Set->Count();

			CComQIPtr<femap::ILayup> Layup_obj;
			Layup_obj = pModel->feLayup;
			
			CComQIPtr<femap::ISet> Elements_Set; //Создание контейнера для хранения элементов
			Elements_Set = pModel->feSet;
			Elements_Set->AddRule(21, femap::zGroupDefinitionType::FGD_ELEM_BYTYPE);
			int elTotalNum = Elements_Set->Count();
			Elements_container.Set(elTotalNum + number);

			int elNumOnLayup = 0;
			CComVariant Elements;

			int sum_of_all_plies = 0;

			Layup_Set->Reset();
			while (Layup_Set->Next() != femap::zReturnCode::FE_FAIL)
			{
				Layup_obj->Get(Layup_Set->CurrentID);
				sum_of_all_plies += Layup_obj->NumberOfPlys;
				if (Layup_obj->NumberOfPlys > total_plies_num)	total_plies_num = Layup_obj->NumberOfPlys;
				
				//Заполняем контейнер [кол-во эл-ов layup-а, элементы...] 
				rc = Elements_Set->clear();
				rc = Elements_Set->AddRule(Layup_obj->ID, femap::zGroupDefinitionType::FGD_ELEM_BYLAYUP);
				rc = Elements_Set->GetArray(&elNumOnLayup, &Elements);
				Elements_map.emplace(Layup_obj->ID, Elements_container.back());
				Elements_container.push_back(elNumOnLayup);
				for (int i = 0; i < elNumOnLayup; i++)
				{
					Elements_container.push_back(((int*)Elements.parray->pvData)[i]);
				}
				Elements.Clear();
			}

			plies.Set(sum_of_all_plies * ply_characteristics_number + 1 * number);

			Layup_Set->Reset();
			while (Layup_Set->Next() != femap::zReturnCode::FE_FAIL)
			{
					Layup_obj->Get(Layup_Set->CurrentID);
					Layups_map.emplace(Layup_Set->CurrentID, plies.back());

					int numplys = 0;
					CComVariant MatID, Thikness, Angle, Globply, FailureTh;
					rc = Layup_obj->GetAllPly2(&numplys, &MatID, &Thikness, &Angle, &Globply, &FailureTh);

					plies.push_back(numplys);
					for (int i = 0; i < numplys; i++)
					{
						plies.push_back(((int*)MatID.parray->pvData)[i]);
						plies.push_back(((double*)Thikness.parray->pvData)[i]);
						plies.push_back(((double*)Angle.parray->pvData)[i]);
						plies.push_back(((int*)Globply.parray->pvData)[i]);
						plies.push_back(((int*)FailureTh.parray->pvData)[i]);

					}

					MatID.Clear();
					Thikness.Clear();
					Angle.Clear();
					Globply.Clear();
					FailureTh.Clear();

			}

		}


		std::unordered_map<int, const double* >::const_iterator Get_Layups_map_cbegin()
		{
			return Layups_map.cbegin();
		}

		std::unordered_map<int, const double* >::const_iterator Get_Layups_map_cend()
		{
			return Layups_map.cend();
		}

		int* Get_FirstElemOnLayup(int layupID)
		{
			return Elements_map[layupID] + 1;
		}

		int Get_number()
		{
			return number;
		}

		int Get_TotalNumberOfplies()
		{
			return total_plies_num;
		}

		int Get_elemNumberOnLayup(int layupID)
		{
			return *Elements_map[layupID];
		}

		int Get_NumberOfplies(int layupID)
		{
			return *Layups_map[layupID];
		}

		int Get_MaterialID(int layupID, int plynum)
		{
			return *(Layups_map[layupID] + (plynum - 1) * 5 + 1);
		}

		double Get_Thikness(int layupID, int plynum)
		{
			return *(Layups_map[layupID] + (plynum - 1) * 5 + 2);
		}

		double Get_Angle(int layupID, int plynum)
		{
			return *(Layups_map[layupID] + (plynum - 1) * 5 + 3);
		}

		int Get_Globalplynum(int layupID, int plynum)
		{
			return *(Layups_map[layupID] + (plynum - 1) * 5 + 4);
		}

		int Get_FailureTheory(int layupID, int plynum)
		{
			return *(Layups_map[layupID] + (plynum - 1) * 5 + 5);
		}

		void Out()
		{
			for (std::unordered_map<int, const double * >::const_iterator it = Layups_map.cbegin(); it != Layups_map.cend(); ++it)
			{
				std::cout << "Layup ID \n" << it->first << " number of plies " << this->Get_NumberOfplies(it->first) << std::endl;
				std::cout << "Materials IDs \n";
				for (int i = 0; i < this->Get_NumberOfplies(it->first); i++)
				{
					std::cout << this->Get_MaterialID(it->first, i) << std::endl;
				}

				std::cout << "Thikness \n";
				for (int i = 0; i < this->Get_NumberOfplies(it->first); i++)
				{
					std::cout << this->Get_Thikness(it->first, i) << std::endl;
				}

				std::cout << "Angles \n";
				for (int i = 0; i < this->Get_NumberOfplies(it->first); i++)
				{
					std::cout << this->Get_Angle(it->first, i) << std::endl;
				}

				std::cout << "Globalplynum \n";
				for (int i = 0; i < this->Get_NumberOfplies(it->first); i++)
				{
					std::cout << this->Get_Globalplynum(it->first, i) << std::endl;
				}		

				std::cout << "FailiureTheory \n";
						for (int i = 0; i < this->Get_NumberOfplies(it->first); i++)
				{
					std::cout << this->Get_FailureTheory(it->first, i) << std::endl;
				}	
					
			}
		}

		void Out_ElNum()
		{
			for (std::unordered_map<int, int* >::const_iterator it = Elements_map.cbegin(); it != Elements_map.cend(); ++it)
			{
				std::cout << "Number " << *it->second << std::endl; 
			}
		}
};

class Properties
{
	private:
		int number;
		std::unordered_map<int, int>Properties_map;

	public:

		void Load(femap::ImodelPtr& pModel, CComQIPtr<femap::ISet> Prop_Set)
		{
			CComQIPtr<femap::IProp> Prop_obj;
			Prop_obj = pModel->feProp;
			
			number = Prop_Set->Count();	

			Prop_Set->Reset();
			while (Prop_Set->Next() != femap::zReturnCode::FE_FAIL)
			{
					Prop_obj->Get(Prop_Set->CurrentID);
					
					Properties_map.emplace(Prop_Set->CurrentID, Prop_obj->layupID);
			}
	
		}

		int Get_LayupID(int propID)
		{
			return Properties_map.at(propID);
		}

		void Out()
		{
			for (std::unordered_map<int, int >::const_iterator it = Properties_map.cbegin(); it != Properties_map.cend(); ++it)
				std::cout << "Prop ID " << it->first << " " << "Layup ID " << it->second << std::endl;
		}

};

class Elements
{
	private:
		int number = 0;
		std::unordered_map<int, int> Elements_map;

	public:

		void Load(femap::ImodelPtr& pModel) // Ошибка выбираются все элементы
		{
			/*
				Для ускорения работы загружаем массивы элементов
				по Property и раскладываем в Elements_map
			*/
			CComQIPtr<femap::ISet> Property_Set;
			Property_Set = pModel->feSet;
			Property_Set->AddRule(21, femap::zGroupDefinitionType::FGD_PROP_BYTYPE);
			int property_SetID = Property_Set->ID; 

			Property_Set->Reset();
			while (Property_Set->Next() != femap::zReturnCode::FE_FAIL)
			{
				int current_PropID = Property_Set->CurrentID;

				CComQIPtr<femap::ISet> Elem_Set;
				Elem_Set = pModel->feSet;
				Elem_Set->AddRule(current_PropID, femap::zGroupDefinitionType::FGD_ELEM_BYPROP);

				int elemNumberOnCurrentProp = 0;
				CComVariant Elements_array;
				Elem_Set->GetArray(&elemNumberOnCurrentProp, &Elements_array);
				number = number + elemNumberOnCurrentProp;

				for (int i = 0; i < elemNumberOnCurrentProp; i++)
				{
					Elements_map.emplace(((int*)Elements_array.parray->pvData)[i], current_PropID);
				}

				Elem_Set->clear();
			}
			
		}

		void Out()
		{
			for (std::unordered_map<int, int>::const_iterator it = Elements_map.cbegin(); it != Elements_map.cend(); ++it)
			{
				std::cout << it->first << "\t" << it->second << std::endl;
			}
		}

		void OutLast()
		{
			std::unordered_map<int, int>::const_iterator it = Elements_map.cend();
			it--;
			std::cout << it->second << std::endl;
			}

		int Get_Number()
		{
			return number;
		}

		int Get_PropertyID(int elemID)
		{
			return Elements_map[elemID];
		}

		std::unordered_map<int, int>::const_iterator cbegin()
		{
			return Elements_map.cbegin();
		}

		std::unordered_map<int, int>::const_iterator cend()
		{
			return Elements_map.cend();
		}

};

class Results
{
	private:

		CComQIPtr<femap::IResults> RBO_1;
		femap::zReturnCode rc;

	public:

		void Load(femap::ImodelPtr& pModel, Layups& Layups)
		{
			RBO_1 = pModel->feResults;

			CComQIPtr<femap::IOutputSet> OutputSetObj;
			OutputSetObj = pModel->feOutputSet;
			int OutputID = OutputSetObj->GetActive();

			int columns_added = 0;
			int total_columns_added = 0;
			CComVariant ColAddedInds;

			for (int current_ply = 1; current_ply < Layups.Get_TotalNumberOfplies() + 1; current_ply++)
			{
				rc = RBO_1->AddColumnV2(OutputID, 1000020 + 500 * (current_ply - 1), FALSE, &columns_added, &ColAddedInds);
				ColAddedInds.Clear();
				total_columns_added += columns_added;
				rc = RBO_1->AddColumnV2(OutputID, 1000021 + 500 * (current_ply - 1), FALSE, &columns_added, &ColAddedInds);
				ColAddedInds.Clear();
				total_columns_added += columns_added;
				rc = RBO_1->AddColumnV2(OutputID, 1000023 + 500 * (current_ply - 1), FALSE, &columns_added, &ColAddedInds);
				ColAddedInds.Clear();
				total_columns_added += columns_added;
			}

			rc = RBO_1->Populate();

		}

		void Get_Values(int elem, int current_ply, double &resSigX, double &resSigY, double &resTau)
		{
			rc = RBO_1->GetValue(elem, current_ply * 3 - 3, &resSigX);
			rc = RBO_1->GetValue(elem, current_ply * 3 - 2, &resSigY);
			rc = RBO_1->GetValue(elem, current_ply * 3 - 1, &resTau);
		}

		void Out(Layups& Layups)
		{
			int propID = 0;
			int layupID = 0;
			int matlID = 0;

			int lauyps_num = Layups.Get_number();
			int plies_num = 0;
			int elem_num = 0;

			double resSigX = 0; double resSigY = 0; double resTau = 0;

			for (std::unordered_map<int, const double* >::const_iterator Layups_iterator = Layups.Get_Layups_map_cbegin(); Layups_iterator != Layups.Get_Layups_map_cend(); ++Layups_iterator)
			{
				plies_num = Layups.Get_NumberOfplies(Layups_iterator->first);
				elem_num = Layups.Get_elemNumberOnLayup(Layups_iterator->first);

				for (int current_ply = 1; current_ply < plies_num+1; current_ply++)
				{
					int* elem = Layups.Get_FirstElemOnLayup(Layups_iterator->first);
					for (int i = 0; i < elem_num; i++)
					{
						Get_Values(elem[i], current_ply, resSigX, resSigY, resTau);

						std::cout << "Ply # " << current_ply << " Elem ID " << elem[i] << " resSigX " << resSigX << " resSigY " << resSigY << " resTau " << resTau << std::endl;
					}
				}
				
			}

		}
};

void Calculate_Hoffman(femap::ImodelPtr& pModel, Elements& Elements, Properties& Properties, Layups& Layups, Materials& Materials)
{
	femap::zReturnCode rc;

	CComQIPtr<femap::IOutputSet> OutputSetObj;
	OutputSetObj = pModel->feOutputSet;
	int OutputID = OutputSetObj->GetActive();

	CComQIPtr<femap::IResults> RBO_2;
	RBO_2 = pModel->feResults;
	CComVariant CComcolIndexes;
	int colInd = 0;
	_bstr_t Title;
	


	Container<double> Results_container;

	CComSafeArray<double> CComSAResults;
	CComSafeArray<int> CComSAElements;

	double	Hoffman_FC = 0;

	int propID = 0;
	int layupID = 0;
	int matlID = 0;

	int lauyps_num = Layups.Get_number();
	int plies_num = 0;
	int elem_num = 0;

	double matSigXT = 0; double matSigYT = 0; double matSigXC = 0; double matSigYC = 0; double matTau = 0;
	double resSigX = 0; double resSigY = 0; double resTau = 0;

	Results Results_obj;
	Results_obj.Load(pModel, Layups);

	std::string Title_str;
	int current_layupID = 0;
	

	for (std::unordered_map<int, const double* >::const_iterator Layups_iterator = Layups.Get_Layups_map_cbegin(); Layups_iterator != Layups.Get_Layups_map_cend(); ++Layups_iterator)
	{
		plies_num = Layups.Get_NumberOfplies(Layups_iterator->first);
		current_layupID = Layups_iterator->first;

		for (int current_ply = 1; current_ply < plies_num + 1; current_ply++)
			{
			Title_str = "Hoffman Fail Ind Layup ";
			Title_str.append(std::to_string(current_layupID));
			Title_str.append(" ply ");
			Title_str.append(std::to_string(current_ply));
			Title = _com_util::ConvertStringToBSTR(Title_str.c_str());
			rc = RBO_2->AddScalarAtElemColumnV2(OutputID, 2000000 + current_ply, Title, femap::zOutputType::FOT_ANY, FALSE, &colInd);
			Title.Detach();
			}
	}

	for (std::unordered_map<int, const double* >::const_iterator Layups_iterator = Layups.Get_Layups_map_cbegin(); Layups_iterator != Layups.Get_Layups_map_cend(); ++Layups_iterator)
			{
				plies_num = Layups.Get_NumberOfplies(Layups_iterator->first);
				elem_num = Layups.Get_elemNumberOnLayup(Layups_iterator->first);

				for (int current_ply = 1; current_ply < plies_num + 1; current_ply++)
				{
					matlID = Layups.Get_MaterialID(Layups_iterator->first, current_ply);
					Materials.Get_Characteristics(matlID, matSigXT, matSigXC, matSigYT, matSigYC, matTau);

					Results_container.Set(Layups.Get_elemNumberOnLayup(Layups_iterator->first));

					int* elem = Layups.Get_FirstElemOnLayup(Layups_iterator->first);
					for (int i = 0; i < elem_num; i++)
					{
						Results_obj.Get_Values(elem[i], current_ply, resSigX, resSigY, resTau);

						Hoffman_FC = (resSigX*resSigX)/(matSigXT*matSigXC) - 
										(resSigX*resSigY)/(matSigXT*matSigXC) +
										(resSigY*resSigY)/(matSigYT*matSigYC) +
										(resTau*resTau)/(matTau*matTau) +
											resSigX*(1/matSigXT - 1/matSigXC)+
											resSigY*(1/matSigYT - 1/matSigYC);

						Results_container.push_back(Hoffman_FC);

					}

					CComSAElements.Add(Layups.Get_elemNumberOnLayup(Layups_iterator->first), Layups.Get_FirstElemOnLayup(Layups_iterator->first), TRUE);
					CComSAResults.Add(Results_container.get_size(), Results_container.front(), TRUE);  

					CComVariant A = CComSAElements;
					CComVariant B = CComSAResults;

					rc = RBO_2->SetScalarAtElemColumnV2(current_ply - 1, elem_num, &A, &B);

					CComSAResults.Destroy();
					CComSAElements.Destroy();
					A.Clear();
					B.Clear();

				};
			}

	rc = RBO_2->Save();
}

void Calculate_Hashin(femap::ImodelPtr& pModel, Elements& Elements, Properties& Properties, Layups& Layups, Materials& Materials)
{
	femap::zReturnCode rc;

	CComQIPtr<femap::IOutputSet> OutputSetObj;
	OutputSetObj = pModel->feOutputSet;
	int OutputID = OutputSetObj->GetActive();

	CComQIPtr<femap::IResults> RBO_2;
	RBO_2 = pModel->feResults;
	CComVariant CComcolIndexes;
	int colInd = 0;
	_bstr_t Title;
	


	Container<double> Results_container;

	CComSafeArray<double> CComSAResults;
	CComSafeArray<int> CComSAElements;

	double	Hoffman_FC = 0;

	double FibFailInd = 0;
	double MatrxFailInd = 0;

	int propID = 0;
	int layupID = 0;
	int matlID = 0;

	int lauyps_num = Layups.Get_number();
	int plies_num = 0;
	int elem_num = 0;

	double matSigXT = 0; double matSigYT = 0; double matSigXC = 0; double matSigYC = 0; double matTau = 0;
	double resSigX = 0; double resSigY = 0; double resTau = 0;

	Results Results_obj;
	Results_obj.Load(pModel, Layups);

	std::string Title_str;
	int current_layupID = 0;
	

	for (std::unordered_map<int, const double* >::const_iterator Layups_iterator = Layups.Get_Layups_map_cbegin(); Layups_iterator != Layups.Get_Layups_map_cend(); ++Layups_iterator)
	{
		plies_num = Layups.Get_NumberOfplies(Layups_iterator->first);
		current_layupID = Layups_iterator->first;

		for (int current_ply = 1; current_ply < plies_num + 1; current_ply++)
			{
			Title_str = "Hasin-Rothem Fail Ind Layup ";
			Title_str.append(std::to_string(current_layupID));
			Title_str.append(" ply ");
			Title_str.append(std::to_string(current_ply));
			Title = _com_util::ConvertStringToBSTR(Title_str.c_str());
			rc = RBO_2->AddScalarAtElemColumnV2(OutputID, 2005000 + current_ply, Title, femap::zOutputType::FOT_ANY, FALSE, &colInd);
			Title.Detach();
			}
	}

	for (std::unordered_map<int, const double* >::const_iterator Layups_iterator = Layups.Get_Layups_map_cbegin(); Layups_iterator != Layups.Get_Layups_map_cend(); ++Layups_iterator)
			{
				plies_num = Layups.Get_NumberOfplies(Layups_iterator->first);
				elem_num = Layups.Get_elemNumberOnLayup(Layups_iterator->first);

				for (int current_ply = 1; current_ply < plies_num + 1; current_ply++)
				{
					matlID = Layups.Get_MaterialID(Layups_iterator->first, current_ply);
					Materials.Get_Characteristics(matlID, matSigXT, matSigXC, matSigYT, matSigYC, matTau);

					Results_container.Set(Layups.Get_elemNumberOnLayup(Layups_iterator->first));

					//Разрушение волокна
					int* elem = Layups.Get_FirstElemOnLayup(Layups_iterator->first);
					for (int i = 0; i < elem_num; i++)
					{
						Results_obj.Get_Values(elem[i], current_ply, resSigX, resSigY, resTau);

						if (resSigX >= 0)

							FibFailInd = (resSigX / matSigXT)*(resSigX / matSigXT) + (resTau / matTau) * (resTau / matTau);
						else

							FibFailInd = (resSigX  / matSigXC) * (resSigX  / matSigXC);

						//Разрушение матрицы
						if (resSigY >= 0)

							MatrxFailInd = (resSigY / matSigYT)*(resSigY / matSigYT) + (resTau / matTau) * (resTau / matTau);
						else

							FibFailInd = (resSigY / matSigYC)*(resSigY / matSigYC) + (resTau / matTau) * (resTau / matTau);

						Results_container.push_back(max(FibFailInd, MatrxFailInd));

					}

					CComSAElements.Add(Layups.Get_elemNumberOnLayup(Layups_iterator->first), Layups.Get_FirstElemOnLayup(Layups_iterator->first), TRUE);
					CComSAResults.Add(Results_container.get_size(), Results_container.front(), TRUE);  

					CComVariant A = CComSAElements;
					CComVariant B = CComSAResults;

					rc = RBO_2->SetScalarAtElemColumnV2(current_ply - 1, elem_num, &A, &B);

					CComSAResults.Destroy();
					CComSAElements.Destroy();
					A.Clear();
					B.Clear();

				};
			}

	rc = RBO_2->Save();
}

void Calculate_Chang(femap::ImodelPtr& pModel, Elements& Elements, Properties& Properties, Layups& Layups, Materials& Materials)
{
	femap::zReturnCode rc;

	CComQIPtr<femap::IOutputSet> OutputSetObj;
	OutputSetObj = pModel->feOutputSet;
	int OutputID = OutputSetObj->GetActive();

	CComQIPtr<femap::IResults> RBO_2;
	RBO_2 = pModel->feResults;
	CComVariant CComcolIndexes;
	int colInd = 0;
	_bstr_t Title;
	


	Container<double> Results_container;

	CComSafeArray<double> CComSAResults;
	CComSafeArray<int> CComSAElements;

	double	Hoffman_FC = 0;

	double FibFailInd = 0;
	double MatrxFailInd = 0;

	int propID = 0;
	int layupID = 0;
	int matlID = 0;

	int lauyps_num = Layups.Get_number();
	int plies_num = 0;
	int elem_num = 0;

	double matSigXT = 0; double matSigYT = 0; double matSigXC = 0; double matSigYC = 0; double matTau = 0;
	double resSigX = 0; double resSigY = 0; double resTau = 0;

	Results Results_obj;
	Results_obj.Load(pModel, Layups);

	std::string Title_str;
	int current_layupID = 0;
	

	for (std::unordered_map<int, const double* >::const_iterator Layups_iterator = Layups.Get_Layups_map_cbegin(); Layups_iterator != Layups.Get_Layups_map_cend(); ++Layups_iterator)
	{
		plies_num = Layups.Get_NumberOfplies(Layups_iterator->first);
		current_layupID = Layups_iterator->first;

		for (int current_ply = 1; current_ply < plies_num + 1; current_ply++)
			{
			Title_str = "Chang-Chang Fail Ind Layup ";
			Title_str.append(std::to_string(current_layupID));
			Title_str.append(" ply ");
			Title_str.append(std::to_string(current_ply));
			Title = _com_util::ConvertStringToBSTR(Title_str.c_str());
			rc = RBO_2->AddScalarAtElemColumnV2(OutputID, 2010000 + current_ply, Title, femap::zOutputType::FOT_ANY, FALSE, &colInd);
			Title.Detach();
			}
	}

	for (std::unordered_map<int, const double* >::const_iterator Layups_iterator = Layups.Get_Layups_map_cbegin(); Layups_iterator != Layups.Get_Layups_map_cend(); ++Layups_iterator)
			{
				plies_num = Layups.Get_NumberOfplies(Layups_iterator->first);
				elem_num = Layups.Get_elemNumberOnLayup(Layups_iterator->first);

				for (int current_ply = 1; current_ply < plies_num + 1; current_ply++)
				{
					matlID = Layups.Get_MaterialID(Layups_iterator->first, current_ply);
					Materials.Get_Characteristics(matlID, matSigXT, matSigXC, matSigYT, matSigYC, matTau);

					Results_container.Set(Layups.Get_elemNumberOnLayup(Layups_iterator->first));

					int* elem = Layups.Get_FirstElemOnLayup(Layups_iterator->first);
					for (int i = 0; i < elem_num; i++)
					{
						Results_obj.Get_Values(elem[i], current_ply, resSigX, resSigY, resTau);

					//Разрушение волокна
						if (resSigX >= 0)

							FibFailInd = (resSigX / matSigXT)*(resSigX / matSigXT) + 0.1 * (resTau / matTau);
						else

							FibFailInd = (resSigX  / matSigXC) * (resSigX  / matSigXC);

					//Разрушение матрицы
						if (resSigY >= 0)

							MatrxFailInd = (resSigY) / (matSigYT * matSigYC) + resSigY*(matSigYC - matSigYT) / (matSigYT * matSigYC) + (resTau / matTau)*(resTau / matTau);

						Results_container.push_back(max(FibFailInd, MatrxFailInd));

					}

					CComSAElements.Add(Layups.Get_elemNumberOnLayup(Layups_iterator->first), Layups.Get_FirstElemOnLayup(Layups_iterator->first), TRUE);
					CComSAResults.Add(Results_container.get_size(), Results_container.front(), TRUE);  

					CComVariant A = CComSAElements;
					CComVariant B = CComSAResults;

					rc = RBO_2->SetScalarAtElemColumnV2(current_ply - 1, elem_num, &A, &B);

					CComSAResults.Destroy();
					CComSAElements.Destroy();
					A.Clear();
					B.Clear();

				};
			}

	rc = RBO_2->Save();
}

void AddColumns()
{
	
}

int main()
{
    clock_t start = clock();
	HRESULT hr;
	hr = CoInitialize(NULL);
	femap::ImodelPtr pModel;
    femap::zReturnCode rc;
	hr = pModel.GetActiveObject("femap.model");
	if (hr != S_OK)
	{
		return 0;//Ends the program if FEMAP isn’t found
	}
	rc = pModel->feAppMessage(femap::zMessageColor(3), "API Started");
	if (rc == femap::zReturnCode::FE_OK) std::cout << "API Started\n";


	CComQIPtr<femap::ISet> Matl_Set;
	Matl_Set = pModel->feSet;
	Matl_Set->AddAll(femap::FT_MATL);

	CComQIPtr<femap::IMatl> Matl_obj;
	Matl_obj = pModel->feMatl;

	Matl_Set->Reset();
	Matl_Set->Next();

	Materials Matl;
	Matl.Load(pModel, Matl_Set);
	Matl.Out();

	std::cout<< "\n";

	CComQIPtr<femap::ISet> Prop_Set;
	Prop_Set = pModel->feSet;
	Prop_Set->AddAll(femap::FT_PROP);

	Properties Prop;
	Prop.Load(pModel, Prop_Set);
	Prop.Out();

	std::cout << Prop.Get_LayupID(3) << std::endl;

	CComQIPtr<femap::ISet> Layup_Set;
	Layup_Set = pModel->feSet;
	Layup_Set->AddAll(femap::FT_LAYUP);

	Layups Layups_obj;
	Layups_obj.Load(pModel, Layup_Set);
	
	Layups_obj.Out();

	Elements Elements_obj;
	Elements_obj.Load(pModel);
	Elements_obj.OutLast();
	std::cout << Elements_obj.Get_Number() << std::endl;

	Layups_obj.Out_ElNum();

	Results Result;
	Result.Load(pModel, Layups_obj);
	//Result.Out(Layups_obj);

	//Расчёт критериев разрушения
	Calculate_Hoffman(pModel, Elements_obj, Prop, Layups_obj, Matl);
	// Calculate_Hashin(pModel, Elements_obj, Prop, Layups_obj, Matl);
	// Calculate_Chang(pModel, Elements_obj, Prop, Layups_obj, Matl);

	clock_t end = clock();
	std::cout << static_cast<double>((end - start))/ CLOCKS_PER_SEC << std::endl;

	std::cout << "program ended\n";
    return 0;
}
